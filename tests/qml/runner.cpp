#include <QtQuickTest/quicktest.h>
#include <qlist.h>
#include <qqml.h>
#include <qtmetamacros.h>

#include <QFile>
#include <QGuiApplication>
#include <QObject>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlError>
#include <QString>
#include <QStringList>
#include <memory>

#include "backend/base.hpp"
#include "models/main_window.hpp"
#include "models/monitor_properties.hpp"
#include "models/profile/profile_editor_controller.hpp"
#include "models/profile/profile_output_properties.hpp"
#include "support/fixture.hpp"
#include "utils/helpers.hpp"

class QmlSetup : public QObject {
    Q_OBJECT
    Q_PROPERTY(MainWindow * controller READ controller NOTIFY controller_changed)
    Q_PROPERTY(QStringList warnings READ warnings NOTIFY warnings_changed)

public:
    ~QmlSetup() override {
        end();
        shutdown_commands();
    }

    Q_SLOT void applicationAvailable() {  // NOLINT: Qt Quick Test hook name.
        qmlRegisterUncreatableMetaObject(
            editor_mode::staticMetaObject,
            "com.nikromen.wayrandr",
            1,
            0,
            "EditorMode",
            "EditorMode only provides enum values"
        );
        qmlRegisterType<MainWindow>("com.nikromen.wayrandr", 1, 0, "MainWindowModel");
        qmlRegisterType<MonitorProperties>("com.nikromen.wayrandr", 1, 0, "MonitorPropertiesModel");
        qmlRegisterType<ProfileEditorController>(
            "com.nikromen.wayrandr", 1, 0, "ProfileEditorControllerModel"
        );
        qmlRegisterType<ProfileOutputProperties>(
            "com.nikromen.wayrandr", 1, 0, "ProfileOutputPropertiesModel"
        );
    }

    Q_SLOT void qmlEngineAvailable(QQmlEngine * engine) {  // NOLINT: Qt Quick Test hook name.
        engine_ = engine;
        connect(engine, &QQmlEngine::warnings, this, [this](const QList<QQmlError> & errors) {
            for (const auto & error : errors) {
                warnings_.append(error.toString());
            }
            emit warnings_changed();
        });
        engine->rootContext()->setContextProperty("harness", this);
    }

    Q_INVOKABLE void begin() {
        end();
        warnings_.clear();
        emit warnings_changed();
        fixture_ = std::make_unique<Fixture>();
        fixture_->profiles();
        output_template_ = fixture_->state()[0];
        window_ = std::make_unique<MainWindow>();
        engine_->rootContext()->setContextProperty("mainWindow", window_.get());
        emit controller_changed();
    }

    Q_INVOKABLE void end() {
        window_.reset();
        fixture_.reset();
        emit controller_changed();
    }

    Q_INVOKABLE void fail_restore(const QString & error) {
        fixture_->write("failures", "1");
        fixture_->write("failure_stderr", error.toStdString());
    }

    Q_INVOKABLE void disconnect_output() { fixture_->write("state", "[]"); }

    Q_INVOKABLE void output_set(const QStringList & names, int scale) {
        auto state = json::array();
        for (const auto & name : names) {
            auto output = output_template_;
            output["name"] = name.toStdString();
            output["scale"] = scale + state.size();
            output["position"]["x"] = state.size() * 2400;
            if (name == "DP-2") {
                output["transform"] = "flipped-90";
            } else if (name == "DP-NEW") {
                output["transform"] = "180";
            }
            state.push_back(output);
        }
        fixture_->write("state", state.dump());
    }

    Q_INVOKABLE void fail_query(bool fail) {
        if (fail) {
            fixture_->write("query_failure", "1");
        } else {
            QFile::remove(fixture_->dir.filePath("query_failure"));
        }
    }

    Q_INVOKABLE void hold_command(const QString & command) { fixture_->hold_command(command); }

    Q_INVOKABLE void release_command(const QString & command) {
        fixture_->release_command(command);
    }

    Q_INVOKABLE [[nodiscard]] bool command_entered(const QString & command) const {
        return fixture_->command_entered(command);
    }

    Q_INVOKABLE void switching_profiles(bool valid) {
        if (!valid) {
            fixture_->write("kanshi/config", "invalid directive\n");
            fixture_->write("auto-wlr-randr/config.toml", "invalid TOML\n");
            return;
        }
        fixture_->write(
            "kanshi/config",
            "profile desk {\n output DP-1 mode 1920x1080@60Hz scale 1\n}\n"
            "profile spare {\n output DP-1 mode 1920x1080@60Hz scale 2\n}\n"
            "profile travel {\n output DP-1 mode 1920x1080@60Hz scale 3\n}\n"
        );
        fixture_->write(
            "auto-wlr-randr/config.toml",
            "[profile.desk]\nsettings = [{ output = 'DP-1', mode = '1920x1080@60Hz', scale = 1 }]\n"
            "[profile.spare]\nsettings = [{ output = 'DP-1', mode = '1920x1080@60Hz', scale = 2 "
            "}]\n"
            "[profile.travel]\nsettings = [{ output = 'DP-1', mode = '1920x1080@60Hz', scale = 3 "
            "}]\n"
        );
    }

    Q_INVOKABLE void empty_auto_config() {
        fixture_->write("auto-wlr-randr/config.toml", "[profile]\n");
    }

    Q_INVOKABLE void change_modes(bool empty, bool disabled) {
        auto state = fixture_->state();
        auto mode = state[0]["modes"][0];
        state[0]["enabled"] = !disabled;
        state[0]["modes"] = json::array();
        if (!empty) {
            mode["width"] = 1280;
            mode["height"] = 720;
            mode["current"] = false;
            state[0]["modes"].push_back(mode);
            if (disabled) {
                mode["width"] = 1920;
                mode["height"] = 1080;
            } else {
                mode["width"] = 2560;
                mode["height"] = 1440;
            }
            mode["current"] = true;
            state[0]["modes"].push_back(mode);
        }
        fixture_->write("state", state.dump());
    }

    Q_INVOKABLE void transform_profiles() {
        fixture_->write("kanshi/config", R"(profile desk {
    output DP-1 mode 1920x1080@60Hz transform 90
    output HDMI-1 mode 1920x1080@60Hz transform 180
    output DP-2 mode 1920x1080@60Hz transform flipped-90
    output HDMI-2 mode 1920x1080@60Hz
}
)");
        fixture_->write("auto-wlr-randr/config.toml", R"([profile.desk]
settings = [
    { output = 'DP-1', mode = '1920x1080@60Hz', transform = '90' },
    { output = 'HDMI-1', mode = '1920x1080@60Hz', transform = '180' },
    { output = 'DP-2', mode = '1920x1080@60Hz', transform = 'flipped-90' },
    { output = 'HDMI-2', mode = '1920x1080@60Hz' },
]
)");
    }

    Q_INVOKABLE [[nodiscard]] bool has_transform(QObject * output) const {
        const auto * model = qobject_cast<ProfileOutputProperties *>(output);
        return model != nullptr && model->to_output().transform.has_value();
    }

    Q_INVOKABLE [[nodiscard]] double scale() const {
        return fixture_->state()[0]["scale"].get<double>();
    }

    [[nodiscard]] auto controller() const -> MainWindow * { return window_.get(); }

    [[nodiscard]] auto warnings() const -> QStringList { return warnings_; }

signals:
    void controller_changed();
    void warnings_changed();

private:
    QStringList warnings_;
    QQmlEngine * engine_ = nullptr;
    std::unique_ptr<Fixture> fixture_;
    std::unique_ptr<MainWindow> window_;
    json output_template_;
};

auto main(int argc, char ** argv) -> int {
    // Quick Test can return before invoking cleanupTestCase (e.g. no test files).
    // Own the application here so fixture cleanup always happens while it is alive.
    QGuiApplication app(argc, argv);
    QmlSetup setup;
    return quick_test_main_with_setup(argc, argv, "wayrandr", QUICK_TEST_SOURCE_DIR, &setup);
}

#include "runner.moc"
