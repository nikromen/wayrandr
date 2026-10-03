#include <qfiledevice.h>
#include <qobject.h>
#include <qtestcase.h>
#include <qtmetamacros.h>
#include <signal.h>  // NOLINT: POSIX kill() requires this C header.

#include <QElapsedTimer>
#include <QFile>
#include <QTest>

#include "models/monitor_block.hpp"
#include "support/environment.hpp"
#include "utils/helpers.hpp"

class CaptureTests : public QObject {
    Q_OBJECT
private slots:

    void cleanupTestCase() { shutdown_commands(); }  // NOLINT: Qt Test hook name.

    void retry_and_cancel() {
        Environment env;
        MonitorBlock block("fake-monitor");
        block.start_capture();
        block.capture_now();
        drain_jobs();
        QVERIFY2(QFile::copy(FAKE_GRIM, env.dir.filePath("grim")), "Fake grim copy");
        QVERIFY2(
            QFile::setPermissions(
                env.dir.filePath("grim"), QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner
            ),
            "Fake grim permissions"
        );
        auto grim_mode = [&](const char * mode) {
            QFile file(env.dir.filePath("grim_mode"));
            QVERIFY2(file.open(QIODevice::WriteOnly | QIODevice::Truncate), "Fake grim mode");
            file.write(mode);
        };
        grim_mode("success");
        block.capture_now();
        QTRY_VERIFY_WITH_TIMEOUT(!block.get_screen_image().isEmpty(), 10000);
        block.stop_capture();
        const auto image = block.get_screen_image();
        grim_mode("hang");
        QFile::remove(env.dir.filePath("grim_pid"));
        block.start_capture();
        block.capture_now();
        QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(env.dir.filePath("grim_pid")), 10000);
        block.stop_capture();
        drain_jobs();
        QVERIFY2(block.get_screen_image() == image, "Cancelled capture delivered image");
        QFile grim_pid(env.dir.filePath("grim_pid"));
        QVERIFY2(grim_pid.open(QIODevice::ReadOnly), "Grim PID");
        QVERIFY2(::kill(grim_pid.readAll().toInt(), 0) == -1, "Cancelled grim still running");
        grim_mode("success");
        block.start_capture();
        block.capture_now();
        drain_jobs();
        block.stop_capture();
    }
};
QTEST_GUILESS_MAIN(CaptureTests)
#include "capture.moc"
