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
        QTest::newRow("desktop") << false;
        QTest::newRow("query-failure") << true;
    }

    void startup() {
        QFETCH(bool, query_failure);
        Fixture fixture;
        fixture.profiles();
        fixture.multi_monitor();
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
        // This is subprocess cleanup, not a test of user closing or monitor rollback.
        process.terminate();
        if (!process.waitForFinished(5000)) {
            process.kill();
            QVERIFY(process.waitForFinished(5000));
        }
    }
};
QTEST_GUILESS_MAIN(StartupTests)
#include "startup.moc"
