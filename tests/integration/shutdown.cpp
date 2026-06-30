#include <linux/bpf_common.h>
#include <linux/filter.h>
#include <linux/prctl.h>
#include <linux/seccomp.h>
#include <qfiledevice.h>
#include <qobject.h>
#include <qtestcase.h>
#include <qtmetamacros.h>
#include <signal.h>  // NOLINT: POSIX kill() requires this C header.
#include <sys/pidfd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>

#include <QByteArray>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QGuiApplication>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSignalSpy>
#include <QString>
#include <QStringList>
#include <QTest>
#include <QTimer>
#include <array>
#include <atomic>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>

#include "models/main_window.hpp"
#include "models/monitor_properties.hpp"
#include "support/environment.hpp"
#include "support/fixture.hpp"
#include "support/process_tree.hpp"
#include "utils/helpers.hpp"

namespace {
void deny_group_kill() {
    // Only this worker and its fake children inherit the filter. GUI-thread test
    // cleanup stays unrestricted. Deny only SIGKILL to a pidfd's process group.
    constexpr auto arg_offset = offsetof(seccomp_data, args);
    constexpr auto signal_offset = arg_offset + sizeof(seccomp_data::args[0]);
    constexpr auto flags_offset = arg_offset + (3 * sizeof(seccomp_data::args[0]));
    std::array<sock_filter, 8> filter = {
        { BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(seccomp_data, nr)),
          BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_pidfd_send_signal, 0, 4),
          BPF_STMT(BPF_LD | BPF_W | BPF_ABS, signal_offset),
          BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SIGKILL, 0, 2),
          BPF_STMT(BPF_LD | BPF_W | BPF_ABS, flags_offset),
          BPF_JUMP(BPF_JMP | BPF_JSET | BPF_K, PIDFD_SIGNAL_PROCESS_GROUP, 1, 0),
          BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
          BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | EPERM) }
    };
    sock_fprog program{ static_cast<std::uint16_t>(filter.size()), filter.data() };
    require(::prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == 0, "Cannot restrict test worker");
    require(
        ::prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &program) == 0,
        "Cannot install cleanup failure filter"
    );
}
}  // namespace

class ShutdownScenario : public QObject {
    Q_OBJECT

public:
    explicit ShutdownScenario(QString scenario)
        : scenario_(std::move(scenario)) {}

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

    void group_shutdown() {
        Environment env;
        auto cancelled = std::make_shared<std::atomic_bool>(false);
        ProcessTreeCleanup cleanup(cancelled);
        const auto pidfile = env.dir.filePath("pid");
        run_job(
            this,
            [pidfile, cancelled] {
                CommandOptions options;
                options.cancelled = cancelled;
                run_command(
                    "/usr/bin/python3",
                    { FAKE_COMMAND, "group-timeout", pidfile.toStdString() },
                    options
                );
                return Completion{};
            },
            [](const std::string &) {}
        );
        QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(pidfile + ".ready"), 10000);
        QFile child_file(pidfile + ".child");
        QVERIFY(child_file.open(QIODevice::ReadOnly));
        const auto child = static_cast<pid_t>(child_file.readAll().toInt());
        QVERIFY(process_is_live(child));
        auto saw_live = std::make_shared<std::atomic_bool>(true);
        auto recovered = std::make_shared<std::atomic_bool>(false);
        run_job(
            nullptr,
            [child, saw_live, recovered] {
                *saw_live = process_is_live(child);
                (void)run_command("/usr/bin/python3", { FAKE_COMMAND, "success" });
                *recovered = true;
                return Completion{};
            },
            [](const std::string &) {}
        );
        shutdown_commands();
        QVERIFY(!process_is_live(child));
        QVERIFY(!saw_live->load());
        QVERIFY(recovered->load());
    }

    void cleanup_failure() {
        Environment env;
        auto cancelled = std::make_shared<std::atomic_bool>(false);
        ProcessTreeCleanup cleanup(cancelled);
        const auto pidfile = env.dir.filePath("pid");
        auto result = std::make_shared<JobResult>();
        run_job(
            this,
            [pidfile, result, cancelled] {
                deny_group_kill();
                CommandOptions options;
                options.cancelled = cancelled;
                try {
                    (void)run_command(
                        "/usr/bin/python3",
                        { FAKE_COMMAND, "group-exit", pidfile.toStdString() },
                        options
                    );
                } catch (const std::exception & e) {
                    result->error = e.what();
                }
                try {
                    result->output = run_command("/usr/bin/python3", { FAKE_COMMAND, "success" });
                } catch (const std::exception & e) {
                    result->output = e.what();
                }
                return [result] { result->done = true; };
            },
            [result](const std::string & error) {
                result->error = error;
                result->done = true;
            }
        );
        QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(pidfile + ".ready"), 10000);
        QFile child_file(pidfile + ".child");
        QVERIFY(child_file.open(QIODevice::ReadOnly));
        const auto child = static_cast<pid_t>(child_file.readAll().toInt());
        QFile release(pidfile + ".release");
        QVERIFY(release.open(QIODevice::WriteOnly));
        release.close();
        QTRY_VERIFY_WITH_TIMEOUT(result->done, 10000);
        QVERIFY2(result->error.find("exit code 17") != std::string::npos, result->error.c_str());
        QVERIFY2(result->error.find("failure detail") != std::string::npos, result->error.c_str());
        QVERIFY2(
            result->error.find("cleanup failed: process group signal failed:") != std::string::npos,
            result->error.c_str()
        );
        QVERIFY2(
            result->output.find("disabled after failed process cleanup") != std::string::npos,
            result->output.c_str()
        );
        QVERIFY(process_is_live(child));
        // Recovery also stays disabled when another mutator might still be alive.
        run_job(
            nullptr,
            [] {
                (void)run_command("/usr/bin/python3", { FAKE_COMMAND, "success" });
                return Completion{};
            },
            [](const std::string &) {}
        );
        shutdown_commands();
        QVERIFY(process_is_live(child));
        // ProcessTreeCleanup terminates and reaps the adopted child on the GUI thread.
    }

    void recovery() {
        Fixture fixture;
        fixture.profiles();
        auto window = std::make_unique<MainWindow>();
        QTRY_VERIFY_WITH_TIMEOUT(!window->is_busy(), 10000);
        auto * monitor = qobject_cast<MonitorProperties *>(window->get_monitors().value(0));
        QVERIFY(monitor != nullptr);
        const bool no_apply = scenario_ == "idle";
        const bool partial = scenario_ == "partial-apply";
        const bool failed_restore = scenario_ == "failed-restore";
        const bool occupied = scenario_ == "occupied" || scenario_ == "quit";
        if (!no_apply) {
            monitor->set_scale(2);
            if (partial) {
                fixture.write("hangs", "1");
            }
            window->apply();
            if (partial) {
                QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(fixture.dir.filePath("pid")), 10000);
                QVERIFY(window->is_busy());
            } else {
                QTRY_VERIFY_WITH_TIMEOUT(!window->is_busy(), 10000);
                QVERIFY(window->is_confirmation_pending());
                QVERIFY(window->is_confirmation_allowed());
            }
            QCOMPARE(fixture.state()[0]["scale"].get<int>(), 2);
        }

        if (scenario_ == "close") {
            QSignalSpy closed(window.get(), &MainWindow::close_ready);
            QVERIFY(!window->prepare_close());
            QTRY_COMPARE_WITH_TIMEOUT(closed.count(), 1, 10000);
            QVERIFY(window->prepare_close());
            QVERIFY(!window->is_confirmation_pending());
            QCOMPARE(fixture.state()[0]["scale"].get<int>(), 1);
        }
        if (scenario_ == "confirmed") {
            window->confirm_apply();
            QTRY_VERIFY_WITH_TIMEOUT(!window->is_busy(), 10000);
            QVERIFY(!window->is_confirmation_pending());
        }
        if (failed_restore) {
            fixture.write("reject_restore", "1");
        }
        if (occupied) {
            run_job(
                this,
                [pidfile = fixture.dir.filePath("blocker_pid").toStdString()] {
                    run_command("/usr/bin/python3", { FAKE_COMMAND, "hang", pidfile });
                    return Completion{};
                },
                [](const std::string &) {}
            );
            QTRY_VERIFY_WITH_TIMEOUT(QFile::exists(fixture.dir.filePath("blocker_pid")), 10000);
        }
        auto queued_ran = std::make_shared<std::atomic_bool>(false);
        if (occupied || partial) {
            run_job(
                this,
                [queued_ran] {
                    *queued_ran = true;
                    return Completion{};
                },
                [](const std::string &) {}
            );
        }
        if (scenario_ == "quit") {
            // Exercise the real application event loop returning without prepare_close().
            QTimer::singleShot(0, QCoreApplication::instance(), &QCoreApplication::quit);
            QCOMPARE(QCoreApplication::exec(), 0);
            QVERIFY(window->is_confirmation_pending());
            QCOMPARE(fixture.state()[0]["scale"].get<int>(), 2);
        }
        int late_callbacks = 0;
        connect(window.get(), &MainWindow::apply_error_changed, this, [&] { ++late_callbacks; });
        connect(window.get(), &MainWindow::confirmation_pending_changed, this, [&] {
            ++late_callbacks;
        });
        // Same lifetime/order as main.cpp: owners die before the executor shuts down.
        window.reset();
        if (occupied || partial) {
            fixture.write("restore_delay", "0.2");
        }
        int ticks = 0;
        QTimer timer;
        connect(&timer, &QTimer::timeout, this, [&] { ++ticks; });
        timer.start(10);
        QElapsedTimer elapsed;
        elapsed.start();
        shutdown_commands();
        QVERIFY(elapsed.elapsed() < 3000);
        QVERIFY(!queued_ran->load());
        QCOMPARE(late_callbacks, 0);
        if (occupied || partial) {
            QVERIFY2(ticks >= 5, "Shutdown recovery blocked GUI event processing");
        }
        int expected_scale = 1;
        if (failed_restore || scenario_ == "confirmed") {
            expected_scale = 2;
        }
        QCOMPARE(fixture.state()[0]["scale"].get<int>(), expected_scale);
        QVERIFY(!QFile::exists(fixture.dir.filePath("overlap")));
        for (const auto * pidfile : { "pid", "blocker_pid", "mutator_pid" }) {
            if (QFile::exists(fixture.dir.filePath(pidfile))) {
                QVERIFY2(
                    ::kill(std::stoi(fixture.read(pidfile)), 0) == -1,
                    "Shutdown leaked a fake process"
                );
            }
        }
        // Assert the full query/mutation order, including interrupted Apply and rollback.
        const auto calls = fixture.read("calls");
        const auto apply = calls.find("2.000000");
        const auto restore = calls.find("1.000000");
        if (no_apply) {
            QVERIFY(apply == std::string::npos);
            QVERIFY(restore == std::string::npos);
        } else {
            QVERIFY(apply != std::string::npos);
            if (scenario_ == "confirmed") {
                QVERIFY(restore == std::string::npos);
            } else {
                QVERIFY(restore != std::string::npos && restore > apply);
                const auto query = calls.find("[\"--json\"]", apply);
                QVERIFY(query != std::string::npos && query < restore);
                QVERIFY(calls.find("2.000000", apply + 1) == std::string::npos);
                QVERIFY(calls.find("1.000000", restore + 1) == std::string::npos);
            }
        }
    }


private:  // NOLINT: Qt moc requires this to end the slots section.
    QString scenario_;
};

class ShutdownTests : public QObject {
    Q_OBJECT
private slots:

    void shutdown_data() {
        QTest::addColumn<QString>("scenario");
        for (const auto * scenario : { "commands",
                                       "occupied",
                                       "quit",
                                       "partial-apply",
                                       "failed-restore",
                                       "close",
                                       "idle",
                                       "confirmed",
                                       "group-shutdown",
                                       "cleanup-failure" }) {
            QTest::newRow(scenario) << QString::fromUtf8(scenario);
        }
    }

    void shutdown() {
        QFETCH(QString, scenario);
        // Each case owns a fresh global executor in a separate process.
        // Also adopt descendants if the scenario times out and QProcess kills it.
        ProcessTreeCleanup cleanup;
        QProcess process;
        process.setProcessEnvironment(QProcessEnvironment::systemEnvironment());
        process.setProcessChannelMode(QProcess::MergedChannels);
        process.start(QCoreApplication::applicationFilePath(), { "--scenario", scenario });
        QVERIFY(process.waitForStarted(5000));
        QTRY_VERIFY_WITH_TIMEOUT(process.state() == QProcess::NotRunning, 30000);
        const auto output = process.readAll();
        QVERIFY2(process.exitStatus() == QProcess::NormalExit, output.constData());
        QVERIFY2(process.exitCode() == 0, output.constData());
        if (scenario == "cleanup-failure") {
            QVERIFY2(
                output.contains("cleanup failed: process group signal failed:"), output.constData()
            );
            QVERIFY2(output.contains("Recovery job failed:"), output.constData());
            QVERIFY2(output.contains("disabled after failed process cleanup"), output.constData());
        } else if (scenario == "failed-restore") {
            QVERIFY2(output.contains("Recovery job failed:"), output.constData());
            QVERIFY2(output.contains("test restoration rejected"), output.constData());
            QVERIFY2(!output.contains("restored after window destruction"), output.constData());
            QVERIFY2(!output.contains("previous configuration restored"), output.constData());
        } else {
            QVERIFY2(!output.contains("Recovery job failed:"), output.constData());
        }
    }
};

auto main(int argc, char * argv[]) -> int {
    QGuiApplication app(argc, argv);
    if (argc == 3 && std::string(argv[1]) == "--scenario") {
        const QString scenario = QString::fromUtf8(argv[2]);
        ShutdownScenario test(scenario);
        QString function = "recovery";
        if (scenario == "commands") {
            function = "cancel_running_and_drop_queued";
        } else if (scenario == "group-shutdown") {
            function = "group_shutdown";
        } else if (scenario == "cleanup-failure") {
            function = "cleanup_failure";
        }
        return QTest::qExec(&test, QStringList{ QString::fromUtf8(argv[0]), function });
    }
    ShutdownTests test;
    return QTest::qExec(&test, argc, argv);
}

#include "shutdown.moc"
