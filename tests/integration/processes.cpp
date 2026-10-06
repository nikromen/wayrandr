#include <qfiledevice.h>
#include <qobject.h>
#include <qtestcase.h>
#include <qtmetamacros.h>
#include <signal.h>  // NOLINT: POSIX kill() requires this C header.

#include <QFile>
#include <QScopeGuard>
#include <QSemaphore>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>
#include <atomic>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "support/environment.hpp"
#include "utils/helpers.hpp"

class ProcessTests : public QObject {
    Q_OBJECT
private slots:

    void cleanupTestCase() { shutdown_commands(); }  // NOLINT: Qt Test hook name.

    void command_data() {
        QTest::addColumn<QString>("mode");
        QTest::addColumn<bool>("success");
        QTest::addColumn<QString>("needle");
        QTest::newRow("success") << "success" << true << "success\n";
        QTest::newRow("empty") << "empty" << true << "";
        QTest::newRow("stderr") << "stderr" << true << "";
        QTest::newRow("exit") << "exit" << false << "exit code 17; stderr: failure detail";
        QTest::newRow("crash") << "crash" << false << "crashed";
        QTest::newRow("hang") << "hang" << false << "timeout after 150 ms";
        QTest::newRow("child") << "child" << false << "timeout after 150 ms";
        QTest::newRow("flood") << "flood" << false << "output limit exceeded";
    }

    void command() {
        QFETCH(QString, mode);
        QFETCH(bool, success);
        QFETCH(QString, needle);
        Environment env;
        QTimer timer;
        QSignalSpy ticks(&timer, &QTimer::timeout);
        timer.start(5);
        CommandOptions options;
        options.timeout_ms = 150;
        options.output_limit = 65536;
        auto result = start_command(
            this,
            "/usr/bin/python3",
            { FAKE_COMMAND, mode.toStdString(), env.dir.filePath("pid").toStdString() },
            options
        );
        QTRY_VERIFY_WITH_TIMEOUT(result->done, 10000);
        QCOMPARE(success, result->error.empty());
        if (success) {
            QCOMPARE(result->output, needle.toStdString());
        } else {
            QVERIFY2(
                result->error.find(needle.toStdString()) != std::string::npos, result->error.c_str()
            );
        }
        if (mode == "hang") {
            QVERIFY2(ticks.count() >= 5, "GUI timers were blocked");
            QFile file(env.dir.filePath("pid"));
            QVERIFY2(file.open(QIODevice::ReadOnly), "No PID file");
            const int pid = file.readAll().toInt();
            QVERIFY2(pid > 0 && ::kill(pid, 0) == -1, "Timed out process still exists");
        }
        if (mode == "child") {
            QFile child_pid(env.dir.filePath("pid.child"));
            QVERIFY2(child_pid.open(QIODevice::ReadOnly), "Child PID missing");
            const auto child = child_pid.readAll().trimmed();
            const auto exited = [child] {
                QFile state("/proc/" + QString::fromUtf8(child) + "/stat");
                if (!state.open(QIODevice::ReadOnly)) {
                    return true;
                }
                // An orphan zombie has exited; its final reap belongs to init.
                return state.readAll().contains(") Z ");
            };
            QTRY_VERIFY_WITH_TIMEOUT(exited(), 10000);
        }
    }

    void missing_program() {
        Environment env;
        auto result = std::make_shared<JobResult>();
        auto available = std::make_shared<bool>(true);
        run_job(
            this,
            [result, available] {
                *available = is_program_available("wayrandr-does-not-exist");
                result->output = run_command("wayrandr-does-not-exist", {});
                return [result] { result->done = true; };
            },
            [result](const std::string & error) {
                result->error = error;
                result->done = true;
            }
        );
        QTRY_VERIFY_WITH_TIMEOUT(result->done, 10000);
        QVERIFY2(!*available, "Availability detection");
        QVERIFY2(result->error.find("failed to start") != std::string::npos, "Start error");
    }

    void lifetime_and_fifo() {
        Environment env;
        auto delivered = std::make_shared<bool>(false);
        auto receiver = std::make_unique<QObject>();
        run_job(
            receiver.get(),
            [delivered] { return [delivered] { *delivered = true; }; },
            [delivered](const std::string &) { *delivered = true; }
        );
        receiver.reset();
        auto order = std::make_shared<std::vector<int>>();
        auto error = std::make_shared<std::string>();
        for (int i = 0; i < 3; ++i) {
            run_job(
                this,
                [order, i] { return [order, i] { order->push_back(i); }; },
                [error](const std::string & message) { *error = message; }
            );
        }
        QTRY_VERIFY_WITH_TIMEOUT(order->size() == 3, 10000);
        QVERIFY2(!*delivered && *order == std::vector<int>({ 0, 1, 2 }), "Lifetime/FIFO failure");
        QVERIFY2(error->empty(), error->c_str());
    }

    void failure_after_receiver_destruction() {
        Environment env;
        auto receiver = std::make_unique<QObject>();
        auto started = std::make_shared<std::atomic_bool>(false);
        auto result = std::make_shared<JobResult>();
        auto released = std::make_shared<QSemaphore>();
        // Unblock before Environment drains jobs, including after an early assertion return.
        const auto release_on_exit = qScopeGuard([released] { released->release(); });
        run_job(
            receiver.get(),
            [started, released]() -> Completion {
                *started = true;
                released->acquire();
                throw std::runtime_error("Failure after receiver destruction");
            },
            [result](const std::string & error) { result->error = error; },
            [result] { result->done = true; }
        );
        QTRY_VERIFY_WITH_TIMEOUT(started->load(), 10000);
        receiver.reset();
        released->release();
        drain_jobs();
        QVERIFY2(result->done, "Shared cleanup must run after the worker fails");
        QVERIFY2(result->error.empty(), "Failure callback reached a destroyed receiver");
    }

    void literal_arguments() {
        Environment env;
        auto result = start_command(
            this, "/usr/bin/python3", { FAKE_COMMAND, "args", "$(touch /tmp/no); * ' spaced" }
        );
        QTRY_VERIFY_WITH_TIMEOUT(result->done, 10000);
        QVERIFY2(result->error.empty(), result->error.c_str());
        QCOMPARE(result->output, std::string("$(touch /tmp/no); * ' spaced\n"));
    }
};
QTEST_GUILESS_MAIN(ProcessTests)
#include "processes.moc"
