#include <qfiledevice.h>
#include <qobject.h>
#include <qtestcase.h>
#include <qtmetamacros.h>
#include <signal.h>  // NOLINT: POSIX kill() requires this C header.

#include <QElapsedTimer>
#include <QFile>
#include <QTest>
#include <atomic>
#include <memory>

#include "support/environment.hpp"
#include "utils/helpers.hpp"

class ShutdownTests : public QObject {
    Q_OBJECT
private slots:

    void cancel_running_and_drop_queued() {
        Environment env;
        // Shutdown cancels an already-running tool and drops queued work.
        QFile::remove(env.dir.filePath("pid"));
        auto queued_ran = std::make_shared<std::atomic_bool>(false);
        run_job(
            this,
            [pidfile = env.dir.filePath("pid").toStdString()] {
                run_command("/usr/bin/python3", { FAKE_COMMAND, "hang", pidfile });
                return Completion{};
            },
            [](const std::string &) {}
        );
        QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(env.dir.filePath("pid")), 10000);
        run_job(
            this,
            [queued_ran] {
                *queued_ran = true;
                return Completion{};
            },
            [](const std::string &) {}
        );
        QElapsedTimer shutdown_time;
        shutdown_time.start();
        shutdown_commands();
        QVERIFY2(
            shutdown_time.elapsed() < 2000 && !queued_ran->load(),
            "Shutdown did not cancel promptly"
        );
        QFile shutdown_pid(env.dir.filePath("pid"));
        QVERIFY2(shutdown_pid.open(QIODevice::ReadOnly), "Shutdown PID missing");
        QVERIFY2(::kill(shutdown_pid.readAll().toInt(), 0) == -1, "Shutdown leaked process");
    }

    void cleanupTestCase() { shutdown_commands(); }  // NOLINT: Qt Test hook name.
};
QTEST_GUILESS_MAIN(ShutdownTests)
#include "shutdown.moc"
