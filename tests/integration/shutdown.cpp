#include <qfiledevice.h>
#include <qobject.h>
#include <qtestcase.h>
#include <qtmetamacros.h>
#include <signal.h>  // NOLINT: POSIX kill() requires this C header.

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
#include <atomic>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>

#include "models/main_window.hpp"
#include "models/monitor_properties.hpp"
#include "support/environment.hpp"
#include "support/fixture.hpp"
#include "utils/helpers.hpp"

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
                                       "confirmed" }) {
            QTest::newRow(scenario) << QString::fromUtf8(scenario);
        }
    }

    void shutdown() {
        QFETCH(QString, scenario);
        // Each case owns a fresh global executor in a separate process.
        QProcess process;
        process.setProcessEnvironment(QProcessEnvironment::systemEnvironment());
        process.setProcessChannelMode(QProcess::MergedChannels);
        process.start(QCoreApplication::applicationFilePath(), { "--scenario", scenario });
        QVERIFY(process.waitForStarted(5000));
        QTRY_VERIFY_WITH_TIMEOUT(process.state() == QProcess::NotRunning, 30000);
        const auto output = process.readAll();
        QVERIFY2(process.exitStatus() == QProcess::NormalExit, output.constData());
        QVERIFY2(process.exitCode() == 0, output.constData());
        if (scenario == "failed-restore") {
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
        }
        return QTest::qExec(&test, QStringList{ QString::fromUtf8(argv[0]), function });
    }
    ShutdownTests test;
    return QTest::qExec(&test, argc, argv);
}

#include "shutdown.moc"
