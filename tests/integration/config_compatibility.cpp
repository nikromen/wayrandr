#include <qfiledevice.h>
#include <qlogging.h>
#include <qobject.h>
#include <qtestcase.h>
#include <qtmetamacros.h>

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QString>
#include <QTest>
#include <string>

#include "backend/auto_wlr_randr/config_repository.hpp"
#include "backend/auto_wlr_randr/conversions.hpp"
#include "backend/kanshi/config_parser.hpp"
#include "backend/kanshi/config_repository.hpp"
#include "backend/kanshi/conversions.hpp"
#include "backend/kanshi/types.hpp"
#include "models/main_window.hpp"
#include "models/profile/profile_output_properties.hpp"
#include "support/fixture.hpp"
#include "utils/helpers.hpp"

class ConfigCompatibilityTests : public QObject {
    Q_OBJECT
    QString kanshi_;
    QString auto_ctl_;

    void validate(const Fixture & fixture, const QString & path, int mode, bool accepted) {
        QFile config(path);
        QVERIFY(config.open(QIODevice::ReadOnly));
        qInfo().noquote() << "Configuration:" << path << '\n' << config.readAll();
        QProcess process;
        auto environment = QProcessEnvironment::systemEnvironment();
        // An absolute, nonexistent socket prevents access to the user's compositor.
        environment.insert("WAYLAND_DISPLAY", fixture.dir.filePath("no-compositor"));
        process.setProcessEnvironment(environment);
        if (mode == 1) {
            process.start(kanshi_, { "--config", path });
        } else {
            process.start(auto_ctl_, { "validate", "--config", path });
        }
        QVERIFY(process.waitForStarted());
        QVERIFY(process.waitForFinished(5000));
        QCOMPARE(process.exitStatus(), QProcess::NormalExit);
        const auto message = process.readAllStandardOutput() + process.readAllStandardError();
        qInfo().noquote() << path << "exit:" << process.exitCode() << message;
        if (mode == 1) {
            // Kanshi has no check-only flag. Successful parsing reaches the deliberate
            // connection failure; parser errors must never be mistaken for success.
            QCOMPARE(process.exitCode(), 1);
            QCOMPARE(message.contains("failed to connect to display"), accepted);
            QCOMPARE(message.contains("failed to parse config file"), !accepted);
        } else if (accepted) {
            QVERIFY2(process.exitCode() == 0, message.constData());
        } else {
            QVERIFY2(process.exitCode() != 0, message.constData());
        }
    }

private slots:

    void initTestCase() {  // NOLINT: Qt Test hook name.
        kanshi_ = QStandardPaths::findExecutable("kanshi");
        auto_ctl_ = QStandardPaths::findExecutable("auto-wlr-randrctl");
        QVERIFY2(!kanshi_.isEmpty(), "Install real Kanshi in the container");
        QVERIFY2(!auto_ctl_.isEmpty(), "Install real auto-wlr-randrctl in the container");
    }

    void cleanupTestCase() { shutdown_commands(); }  // NOLINT: Qt Test hook name.

    void corpus_data() {
        QTest::addColumn<QString>("seed");
        QTest::addColumn<bool>("accepted");
        QTest::newRow("desktop") << QString("desktop") << true;
        QTest::newRow("directives") << QString("directives") << true;
        QTest::newRow("quoted-tokens") << QString("quoted_tokens") << true;
        QTest::newRow("numeric") << QString("numeric") << true;
        QTest::newRow("incomplete") << QString("incomplete") << false;
        QTest::newRow("unknown-directive") << QString("quoted_directive") << false;
    }

    void corpus() {
        QFETCH(QString, seed);
        QFETCH(bool, accepted);
        Fixture fixture;
        const auto path = QDir(KANSHI_CORPUS).filePath(seed);
        validate(fixture, path, 1, accepted);
        QFile input(path);
        QVERIFY(input.open(QIODevice::ReadOnly));
        try {
            const auto parsed = kanshi_config_parser::parse(input.readAll().toStdString());
            fixture.write("serialized", kanshi_config_parser::serialize(parsed));
            validate(fixture, fixture.dir.filePath("serialized"), 1, accepted);
        } catch (const kanshi_config_parser::ParseError &) {
            QVERIFY(!accepted);
        }
    }

    void serialized_values() {
        Fixture fixture;
        KanshiConfig config;
        KanshiProfile profile;
        profile.id = "--force \"quoted\" \\ Příliš 🖥";
        KanshiOutputSetting output;
        output.criteria = "Panel\" } profile injected { exec payload #\\\t🖥";
        output.enabled = true;
        profile.outputs.push_back(output);
        // These are parsed only: the nonexistent compositor prevents activation.
        profile.exec = { R"(echo "quoted" 'single' \{ \} $HOME; echo $(payload))" };
        config.profiles.push_back(profile);
        fixture.write("serialized", kanshi_config_parser::serialize(config));
        validate(fixture, fixture.dir.filePath("serialized"), 1, true);
    }

    void complex_roundtrip_data() {
        QTest::addColumn<int>("mode");
        QTest::addColumn<std::string>("content");
        QTest::newRow("kanshi") << 1 << std::string(R"(include /dev/null
output "Panel 🖥" {
    alias $panel
    disable
    adaptive_sync on
}
# Profile priority and unnamed profile identity must survive saving.
profile "z first" {
    ...output $panel {
        scale 0x1.8p+0
        transform flipped-270
    }
    exec echo 'one'"two" \{ \} '$HOME; $(payload)' # shell comment
}
profile {
    output DP-2 mode --custom 1280x720@75Hz position -10,+20
}
)");
        QTest::newRow("auto-wlr-randr") << 2 << std::string(R"(on_no_match_exec = ["""echo
second line 🖥"""]
[profile.z_first]
exec = ['echo "quotes"', 'echo \\path']
settings = [{ output = 'DP-1', preferred = false, mode = '1920x1080', right_of = 'HDMI-1', scale = 1 }]
[profile.a_second]
settings = [{ output = 'DP-2', on = true, adaptive_sync = false, transform = 'normal' }]
)");
    }

    void complex_roundtrip() {
        QFETCH(int, mode);
        QFETCH(std::string, content);
        Fixture fixture;
        fixture.write("source", content);
        const auto path = fixture.dir.filePath("source");
        validate(fixture, path, mode, true);
        if (mode == 1) {
            KanshiConfigRepository repository(path.toStdString());
            const auto loaded = repository.load();
            const auto document = kanshi_conversions::to_profile_document(loaded);
            (void)repository.save(kanshi_conversions::from_profile_document(document));
        } else {
            AutoWlrRandrConfigRepository repository(path.toStdString());
            const auto loaded = repository.load();
            const auto document = auto_wlr_randr_conversions::to_profile_document(loaded);
            (void)repository.save(auto_wlr_randr_conversions::from_profile_document(document));
        }
        validate(fixture, path, mode, true);
    }

    void profile_data() {
        QTest::addColumn<int>("mode");
        QTest::newRow("kanshi") << 1;
        QTest::newRow("auto-wlr-randr") << 2;
    }

    void profile() {
        QFETCH(int, mode);
        Fixture fixture;
        fixture.profiles();
        QString name = "kanshi/config";
        if (mode == 2) {
            name = "auto-wlr-randr/config.toml";
        }
        const auto path = fixture.dir.filePath(name);
        validate(fixture, path, mode, true);
        MainWindow window;
        QTRY_VERIFY_WITH_TIMEOUT(!window.is_busy(), 20000);
        window.set_backend_mode(mode);
        QTRY_VERIFY_WITH_TIMEOUT(!window.is_busy(), 20000);
        auto * editor = window.get_profile_editor();
        QVERIFY(editor->select_profile("desk"));
        auto * output = qobject_cast<ProfileOutputProperties *>(editor->get_outputs().value(0));
        QVERIFY(output != nullptr);
        output->set_scale(1.5F);
        output->set_mode("1280x720@75Hz");
        output->set_transform("90");
        output->set_adaptive_sync(true);
        editor->save_profile();
        QTRY_VERIFY_WITH_TIMEOUT(!window.is_busy(), 20000);
        QVERIFY(!editor->is_dirty());
        validate(fixture, path, mode, true);
        editor->duplicate_profile("desk", "copy");
        editor->save_profile();
        QTRY_VERIFY_WITH_TIMEOUT(!window.is_busy(), 20000);
        validate(fixture, path, mode, true);
        editor->delete_profile("copy");
        QTRY_VERIFY_WITH_TIMEOUT(!window.is_busy(), 20000);
        validate(fixture, path, mode, true);
        editor->create_profile_from_live("live");
        QTRY_VERIFY_WITH_TIMEOUT(!window.is_busy(), 20000);
        auto * live_output =
            qobject_cast<ProfileOutputProperties *>(editor->get_outputs().value(0));
        QVERIFY(live_output != nullptr);
        QCOMPARE(live_output->get_mode(), QStringLiteral("1920x1080@60.000000Hz"));
        editor->save_profile();
        QTRY_VERIFY_WITH_TIMEOUT(!window.is_busy(), 20000);
        QVERIFY2(!editor->is_dirty(), qPrintable(editor->get_save_status_text()));
        validate(fixture, path, mode, true);
    }

    void invalid_auto_config_data() {
        QTest::addColumn<QString>("content");
        QTest::newRow("syntax") << QString("profile broken {");
        QTest::newRow("type") << QString("[profile.bad]\n[[profile.bad.settings]]\noutput = 42\n");
        QTest::newRow("unknown-root") << QString("future = 1\n[profile.p]\n");
        QTest::newRow("unknown-output")
            << QString("[profile.p]\nsettings = [{ output = 'DP-1', future = 1 }]\n");
        QTest::newRow("missing-profile") << QString("on_no_match_exec = []\n");
    }

    void invalid_auto_config() {
        QFETCH(QString, content);
        Fixture fixture;
        fixture.write("bad.toml", content.toStdString());
        validate(fixture, fixture.dir.filePath("bad.toml"), 2, false);
    }
};

QTEST_MAIN(ConfigCompatibilityTests)
#include "config_compatibility.moc"
