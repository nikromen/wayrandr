#include <qobject.h>
#include <qtestcase.h>
#include <qtestsupport_core.h>
#include <qtmetamacros.h>

#include <QByteArray>
#include <QProcess>
#include <QProcessEnvironment>
#include <QString>
#include <QTest>
#include <string>

#include "support/fixture.hpp"
#include "utils/helpers.hpp"

class StartupTests : public QObject {
    Q_OBJECT
private slots:

    void cleanupTestCase() { shutdown_commands(); }  // NOLINT: Qt Test hook name.

    void startup_data() {
        QTest::addColumn<bool>("query_failure");
        QTest::addColumn<bool>("invalid_transform");
        QTest::newRow("desktop") << false << false;
        QTest::newRow("query-failure") << true << false;
        QTest::newRow("direct-invalid-transform") << false << true;
    }

    void startup() {
        QFETCH(bool, query_failure);
        QFETCH(bool, invalid_transform);
        Fixture fixture;
        fixture.profiles();
        fixture.multi_monitor();
        const std::string broken =
            "[profile.desk]\n"
            "settings = [{ output = 'DP-1', transform = 'invalid' }]\n";
        if (invalid_transform) {
            fixture.write("auto-wlr-randr/config.toml", broken);
        }
        const auto initial_state = fixture.read("state");
        if (query_failure) {
            fixture.write("query_failure", "1");
        }
        QProcess process;
        process.setProcessEnvironment(QProcessEnvironment::systemEnvironment());
        process.setProcessChannelMode(QProcess::MergedChannels);
        process.start(QStringLiteral(WAYRANDR_APP), {});
        QVERIFY(process.waitForStarted(5000));
        QByteArray output;
        QTRY_VERIFY_WITH_TIMEOUT(
            (output += process.readAll()).contains("UI loaded successfully"), 10000
        );
        QTRY_VERIFY_WITH_TIMEOUT(!fixture.read("calls").empty(), 10000);
        QTest::qWait(500);
        output += process.readAll();
        QCOMPARE(process.state(), QProcess::Running);
        QVERIFY2(!output.contains("ReferenceError"), output.constData());
        QVERIFY2(!output.contains("TypeError"), output.constData());
        QVERIFY2(!output.contains("is not installed"), output.constData());
        QVERIFY2(!output.toLower().contains("binding loop"), output.constData());
        QVERIFY2(!output.contains("Failed to load QML"), output.constData());
        QVERIFY(fixture.read("calls").find("--json") != std::string::npos);
        if (query_failure) {
            QVERIFY2(output.contains("exit code 1"), output.constData());
        }
        if (invalid_transform) {
            QVERIFY2(output.contains("Failed to load profile config"), output.constData());
            QVERIFY2(output.contains("Unsupported transform 'invalid'"), output.constData());
            QVERIFY2(output.contains("profile 'desk'"), output.constData());
            QVERIFY2(output.contains("output 'DP-1'"), output.constData());
            QCOMPARE(fixture.read("auto-wlr-randr/config.toml"), broken);
            QCOMPARE(fixture.read("state"), initial_state);
            // Startup loads all backend views while MainWindow defaults to direct mode.
            // Only queries are permitted; no Apply, daemon reload or profile activation.
            const auto calls = fixture.read("calls");
            QVERIFY(calls.find("--output") == std::string::npos);
            const auto daemon_calls = fixture.read("daemon_calls");
            QVERIFY(daemon_calls.find("reload") == std::string::npos);
            QVERIFY(daemon_calls.find("switch") == std::string::npos);
        }
        // This is subprocess cleanup, not a test of user closing or monitor rollback.
        process.terminate();
        if (!process.waitForFinished(5000)) {
            process.kill();
            QVERIFY(process.waitForFinished(5000));
        }
        if (invalid_transform) {
            QCOMPARE(fixture.read("auto-wlr-randr/config.toml"), broken);
            QCOMPARE(fixture.read("state"), initial_state);
        }
    }
};
QTEST_GUILESS_MAIN(StartupTests)
#include "startup.moc"
