#include <QtQuickTest/quicktest.h>
#include <qlist.h>
#include <qqml.h>
#include <qtmetamacros.h>

#include <QGuiApplication>
#include <QObject>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlError>
#include <QString>
#include <QStringList>
#include <memory>

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
        window_ = std::make_unique<MainWindow>();
        engine_->rootContext()->setContextProperty("mainWindow", window_.get());
        emit controller_changed();
    }

    Q_INVOKABLE void end() {
        window_.reset();
        fixture_.reset();
        emit controller_changed();
    }

    Q_INVOKABLE void fail_restore() { fixture_->write("failures", "1"); }

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
};

auto main(int argc, char ** argv) -> int {
    // Quick Test can return before invoking cleanupTestCase (e.g. no test files).
    // Own the application here so fixture cleanup always happens while it is alive.
    QGuiApplication app(argc, argv);
    QmlSetup setup;
    return quick_test_main_with_setup(argc, argv, "wayrandr", QUICK_TEST_SOURCE_DIR, &setup);
}

#include "runner.moc"
