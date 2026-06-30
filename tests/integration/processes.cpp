#include <qfiledevice.h>
#include <qobject.h>
#include <qtestcase.h>
#include <qtmetamacros.h>
#include <signal.h>  // NOLINT: POSIX kill() requires this C header.
#include <sys/types.h>

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
#include "support/process_tree.hpp"
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
        QTest::newRow("wait-children") << "wait-children" << true << "success\n";
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
        ProcessTreeCleanup cleanup;
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

    void descendants_data() {
        QTest::addColumn<QString>("mode");
        QTest::addColumn<QString>("needle");
        QTest::newRow("exit") << "group-exit" << "exit code 17; stderr: failure detail";
        QTest::newRow("crash") << "group-crash" << "crashed";
        QTest::newRow("timeout") << "group-timeout" << "timeout after 2000 ms";
        QTest::newRow("cancel") << "group-cancel" << "cancelled";
        QTest::newRow("flood") << "group-flood" << "output limit exceeded";
        QTest::newRow("success") << "group-success" << "";
    }

    void descendants() {
        QFETCH(QString, mode);
        QFETCH(QString, needle);
        Environment env;
        auto cancelled = std::make_shared<std::atomic_bool>(false);
        ProcessTreeCleanup cleanup(cancelled);
        CommandOptions options;
        options.cancelled = cancelled;
        options.timeout_ms = 2000;
        options.output_limit = 65536;
        const auto pidfile = env.dir.filePath("pid");
        auto result = start_command(
            this,
            "/usr/bin/python3",
            { FAKE_COMMAND, mode.toStdString(), pidfile.toStdString() },
            options
        );
        QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(pidfile + ".ready"), 10000);
        QFile child_file(pidfile + ".child");
        QVERIFY(child_file.open(QIODevice::ReadOnly));
        const auto child = static_cast<pid_t>(child_file.readAll().toInt());
        QVERIFY(process_is_live(child));
        QFile release(pidfile + ".release");
        QVERIFY(release.open(QIODevice::WriteOnly));
        release.close();
        if (mode == "group-cancel") {
            *cancelled = true;
        }
        QTRY_VERIFY_WITH_TIMEOUT(result->done, 10000);
        if (mode == "group-success") {
            QVERIFY2(result->error.empty(), result->error.c_str());
            QVERIFY2(process_is_live(child), "Successful commands must retain legitimate children");
            return;
        }
        QVERIFY2(
            result->error.find(needle.toStdString()) != std::string::npos, result->error.c_str()
        );
        QVERIFY2(!process_is_live(child), "Failure returned with a live descendant");
        // The next serialized operation observes cleanup, rather than merely waiting
        // for a later death of the descendant after the first command returned.
        auto observed_live = std::make_shared<bool>(true);
        auto next = std::make_shared<JobResult>();
        run_job(
            this,
            [child, observed_live, next] {
                *observed_live = process_is_live(child);
                auto output = run_command("/usr/bin/python3", { FAKE_COMMAND, "success" });
                return [next, output] {
                    next->output = output;
                    next->done = true;
                };
            },
            [next](const std::string & error) {
                next->error = error;
                next->done = true;
            }
        );
        QTRY_VERIFY_WITH_TIMEOUT(next->done, 10000);
        QVERIFY2(next->error.empty(), next->error.c_str());
        QVERIFY(!*observed_live);
        QCOMPARE(next->output, std::string("success\n"));
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

    void failed_exec() {
        Environment env;
        ProcessTreeCleanup cleanup;
        const auto executable = env.dir.filePath("bad-interpreter");
        QFile file(executable);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QVERIFY(file.write("#!/nonexistent-wayrandr-test-interpreter\n") > 0);
        file.close();
        QVERIFY(file.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        auto result = start_command(this, executable.toStdString(), {});
        QTRY_VERIFY_WITH_TIMEOUT(result->done, 10000);
        QVERIFY2(result->error.find("failed to start") != std::string::npos, result->error.c_str());
        auto next = start_command(this, "/usr/bin/python3", { FAKE_COMMAND, "success" });
        QTRY_VERIFY_WITH_TIMEOUT(next->done, 10000);
        QVERIFY2(next->error.empty(), next->error.c_str());
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
