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

#include "backend/kanshi/config_parser.hpp"
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
    }

    void invalid_auto_config_data() {
        QTest::addColumn<QString>("content");
        QTest::newRow("syntax") << QString("profile broken {");
        QTest::newRow("type") << QString("[profile.bad]\n[[profile.bad.settings]]\noutput = 42\n");
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
