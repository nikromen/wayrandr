#include "models/main_window.hpp"

#include <qcontainerfwd.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qtenvironmentvariables.h>
#include <qtestcase.h>
#include <qtmetamacros.h>

#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QGuiApplication>
#include <QMetaObject>
#include <QSignalSpy>
#include <QString>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QVariant>
#include <QVariantMap>
#include <cstddef>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

#include "models/monitor_properties.hpp"
#include "support/environment.hpp"
#include "support/fixture.hpp"
#include "utils/helpers.hpp"

#define SETTLE(window) QTRY_VERIFY_WITH_TIMEOUT(!(window).is_busy(), 20000)

namespace {
auto draft_state(const MonitorProperties & monitor) -> QVariantMap {
    QVariantMap state;
    for (const auto * name : { "enabled",
                               "adaptiveSync",
                               "activeResolutionIndex",
                               "scale",
                               "transform",
                               "positionX",
                               "positionY" }) {
        state.insert(QString::fromUtf8(name), monitor.property(name));
    }
    return state;
}

void verify_edits_locked(MainWindow & window, MonitorProperties & monitor) {
    QVERIFY(!window.is_direct_editing_allowed());
    const auto before = draft_state(monitor);
    monitor.set_scale(2);
    monitor.set_transform("flipped-180");
    monitor.set_active_resolution_index(0);
    monitor.activate_preferred_mode();
    monitor.set_position_x(400);
    monitor.set_position_y(500);
    monitor.set_adaptive_sync(true);
    monitor.start_drag(0, 0);
    monitor.update_drag(400, 500, 1, &window, 2000, 2000);
    window.reset_canvas_layout(2000, 1);
    monitor.set_enabled(false);
    QCOMPARE(draft_state(monitor), before);
}
}  // namespace

class MainWindowTests : public QObject {
    Q_OBJECT
private slots:

    void cleanupTestCase() { shutdown_commands(); }  // NOLINT: Qt Test hook name.

    void invalid_mode_selection() {
        Fixture fixture;
        MainWindow window;
        SETTLE(window);
        auto * monitor = qobject_cast<MonitorProperties *>(window.get_monitors().value(0));
        QVERIFY(monitor != nullptr);
        QSignalSpy changes(monitor, &MonitorProperties::active_resolution_index_changed);
        // ComboBox.currentIndex is -1 when its model has no selection.
        QVERIFY(monitor->setProperty("activeResolutionIndex", -1));
        QCOMPARE(monitor->get_active_resolution_index(), size_t{ 0 });
        monitor->set_active_resolution_index(monitor->get_resolutions().size());
        QCOMPARE(monitor->get_active_resolution_index(), size_t{ 0 });
        QCOMPARE(monitor->get_resolution_width(), 1920);
        QCOMPARE(monitor->get_resolution_height(), 1080);
        QCOMPARE(changes.count(), 0);
    }

    void empty_modes() {
        Fixture fixture;
        auto state = fixture.state();
        state[0]["modes"] = json::array();
        fixture.write("state", state.dump());
        MainWindow window;
        SETTLE(window);
        auto * monitor = qobject_cast<MonitorProperties *>(window.get_monitors().value(0));
        QVERIFY(monitor != nullptr);
        QVERIFY(monitor->get_resolutions().isEmpty());
        QCOMPARE(monitor->property("resolutionWidth").toInt(), 0);
        QCOMPARE(monitor->property("resolutionHeight").toInt(), 0);
    }

    void disconnected_output() {
        Fixture f;

        f.reset();
        auto state = f.state();
        auto second = state[0];
        second["name"] = "DP-2";
        state.push_back(second);
        f.write("state", state.dump());
        MainWindow window;
        SETTLE(window);
        auto * missing = qobject_cast<MonitorProperties *>(window.get_monitors()[1]);
        QVERIFY2(missing != nullptr, "Second monitor model available");
        window.apply();
        SETTLE(window);
        state.erase(1);
        f.write("state", state.dump());
        window.cancel_apply();
        SETTLE(window);
        QVERIFY2(
            !window.is_confirmation_pending() && !missing->is_enabled(),
            "Successful rollback must disable disconnected output in UI"
        );
    }

    void complete_transaction_data() {
        QTest::addColumn<bool>("confirm");
        QTest::newRow("confirm") << true;
        QTest::newRow("cancel") << false;
    }

    void complete_transaction() {
        QFETCH(bool, confirm);
        Fixture f;
        f.multi_monitor();
        const auto original = f.state();
        MainWindow window;
        SETTLE(window);
        QCOMPARE(window.get_monitors().size(), 2);
        for (auto * object : window.get_monitors()) {
            auto * monitor = qobject_cast<MonitorProperties *>(object);
            QVERIFY(monitor != nullptr);
            monitor->set_enabled(true);
            monitor->set_active_resolution_index(1);
            monitor->set_scale(2);
            monitor->set_transform("90");
            monitor->set_position_x(720);
            monitor->set_position_y(120);
            monitor->set_adaptive_sync(true);
        }
        auto expected = original;
        for (auto & monitor : expected) {
            monitor["enabled"] = true;
            monitor["modes"][0]["current"] = false;
            monitor["modes"][1]["current"] = true;
            monitor["scale"] = 2;
            monitor["transform"] = "90";
            monitor["position"] = { { "x", 720 }, { "y", 120 } };
            monitor["adaptive_sync"] = true;
        }
        window.apply();
        SETTLE(window);
        QVERIFY(window.is_confirmation_allowed());
        QVERIFY(f.state() == expected);
        if (confirm) {
            window.confirm_apply();
        } else {
            window.cancel_apply();
            // Disabled outputs have no active settings in the external protocol.
            const auto disabled = expected[1];
            expected = original;
            expected[1] = disabled;
            expected[1]["enabled"] = false;
        }
        SETTLE(window);
        QVERIFY(!window.is_confirmation_pending());
        QVERIFY(f.state() == expected);
        QCOMPARE(
            qobject_cast<MonitorProperties *>(window.get_monitors()[1])->is_enabled(), confirm
        );
    }

    void apply_confirm_cancel() {
        Fixture f;

        f.reset();
        MainWindow window;
        SETTLE(window);
        auto * monitor = qobject_cast<MonitorProperties *>(window.get_monitors()[0]);
        QVERIFY2(monitor != nullptr, "Monitor model available");
        monitor->set_scale(2);
        window.apply();
        SETTLE(window);
        QVERIFY2(f.state()[0]["scale"] == 2, "UI applies edited state");
        QVERIFY2(
            window.is_confirmation_pending() && window.is_confirmation_allowed(), "UI Apply state"
        );
        QVERIFY2(window.get_confirmation_seconds_left() == 15, "Countdown starts");
        QVERIFY2(
            !window.can_switch_backend(1) && !window.can_switch_backend(2),
            "UI blocks switching away"
        );
        window.set_backend_mode(2);
        window.apply();
        SETTLE(window);
        QVERIFY2(window.get_backend_mode() == 0, "Pending backend remains active");
        window.confirm_apply();
        SETTLE(window);
        QVERIFY2(
            !window.is_confirmation_pending() && window.get_confirmation_seconds_left() == 0,
            "Confirm stops countdown"
        );
        window.apply();
        SETTLE(window);
        window.cancel_apply();
        SETTLE(window);
        QVERIFY2(!window.is_confirmation_pending(), "UI manual cancel");
    }

    void reentrant_apply_admission() {
        Fixture fixture;
        MainWindow window;
        SETTLE(window);
        auto * monitor = qobject_cast<MonitorProperties *>(window.get_monitors()[0]);
        monitor->set_scale(2);
        bool attempted = false;
        bool gate_closed = false;
        connect(&window, &MainWindow::apply_error_changed, &window, [&] {
            if (!attempted && window.get_apply_error().isEmpty()) {
                attempted = true;
                gate_closed = window.is_busy();
                window.apply();
            }
        });
        fixture.hold_command("mutation");
        window.apply();
        QTRY_VERIFY_WITH_TIMEOUT(fixture.command_entered("mutation"), 10000);
        fixture.release_command("mutation");
        drain_jobs();
        QVERIFY(attempted);
        QVERIFY2(gate_closed, "Apply must close admission before emitting error notifications");
        QVERIFY(window.get_apply_error().isEmpty());
        QVERIFY(window.is_confirmation_allowed());
        QCOMPARE(fixture.state()[0]["scale"].get<int>(), 2);
        window.cancel_apply();
        SETTLE(window);
        QCOMPARE(fixture.state()[0]["scale"].get<int>(), 1);
    }

    void reentrant_confirmation_data() {
        QTest::addColumn<QString>("action");
        for (const auto * action : { "confirm", "cancel", "close" }) {
            QTest::newRow(action) << QString::fromUtf8(action);
        }
    }

    void reentrant_confirmation() {
        QFETCH(QString, action);
        Fixture fixture;
        MainWindow window;
        SETTLE(window);
        auto * monitor = qobject_cast<MonitorProperties *>(window.get_monitors()[0]);
        monitor->set_scale(2);
        QSignalSpy closed(&window, &MainWindow::close_ready);
        bool attempted = false;
        int countdown_on_admission = 0;
        connect(&window, &MainWindow::confirmation_pending_changed, &window, [&] {
            if (!attempted && window.is_confirmation_allowed()) {
                attempted = true;
                countdown_on_admission = window.get_confirmation_seconds_left();
                if (action == "confirm") {
                    window.confirm_apply();
                } else if (action == "cancel") {
                    window.cancel_apply();
                } else {
                    window.prepare_close();
                }
            }
        });
        window.apply();
        QTRY_VERIFY_WITH_TIMEOUT(attempted, 10000);
        SETTLE(window);
        QCOMPARE(countdown_on_admission, 15);
        QCOMPARE(window.get_confirmation_seconds_left(), 0);
        for (auto * timer : window.findChildren<QTimer *>(QString(), Qt::FindDirectChildrenOnly)) {
            QVERIFY(!timer->isActive());
        }
        QVERIFY(!window.is_confirmation_pending());
        QVERIFY(window.is_direct_editing_allowed());
        int expected_scale = 1;
        if (action == "confirm") {
            expected_scale = 2;
        }
        QCOMPARE(fixture.state()[0]["scale"].get<int>(), expected_scale);
        if (action == "close") {
            QCOMPARE(closed.count(), 1);
            QVERIFY(window.prepare_close());
            // Closing skips the GUI refresh after restoring the live configuration.
            QCOMPARE(monitor->get_scale(), 2.0F);
        } else {
            QCOMPARE(monitor->get_scale(), static_cast<float>(expected_scale));
        }
        QVERIFY(!QFile::exists(fixture.dir.filePath("overlap")));
    }

    void reentrant_deferred_cancel() {
        Fixture fixture;
        MainWindow window;
        SETTLE(window);
        auto * monitor = qobject_cast<MonitorProperties *>(window.get_monitors()[0]);
        monitor->set_scale(2);
        int confirmable = 0;
        connect(&window, &MainWindow::confirmation_pending_changed, &window, [&] {
            if (window.is_confirmation_allowed()) {
                ++confirmable;
                window.confirm_apply();
            }
        });
        fixture.hold_command("mutation");
        window.apply();
        QTRY_VERIFY_WITH_TIMEOUT(fixture.command_entered("mutation"), 10000);
        window.cancel_apply();
        fixture.release_command("mutation");
        SETTLE(window);
        QCOMPARE(confirmable, 0);
        QVERIFY(!window.is_confirmation_pending());
        QVERIFY(window.is_direct_editing_allowed());
        QCOMPARE(monitor->get_scale(), 1.0F);
        QCOMPARE(fixture.state()[0]["scale"].get<int>(), 1);
        const auto calls = fixture.read("calls");
        const auto apply = calls.find("2.000000");
        const auto restore = calls.find("1.000000", apply);
        QVERIFY(apply != std::string::npos);
        QVERIFY(restore != std::string::npos && restore > apply);
        QVERIFY(calls.find("2.000000", restore) == std::string::npos);
        QVERIFY(!QFile::exists(fixture.dir.filePath("overlap")));
    }

    void reentrant_restore_failure_close() {
        Fixture fixture;
        MainWindow window;
        SETTLE(window);
        auto * monitor = qobject_cast<MonitorProperties *>(window.get_monitors()[0]);
        monitor->set_scale(2);
        window.apply();
        SETTLE(window);
        fixture.write("failures", "1");
        QSignalSpy closed(&window, &MainWindow::close_ready);
        bool attempted = false;
        bool diagnostic_ready = false;
        connect(&window, &MainWindow::confirmation_pending_changed, &window, [&] {
            if (!attempted && !window.is_busy() && window.is_confirmation_pending() &&
                !window.is_confirmation_allowed()) {
                attempted = true;
                diagnostic_ready = window.get_apply_error().contains("Restore failed");
                window.prepare_close();
            }
        });
        window.cancel_apply();
        QTRY_VERIFY_WITH_TIMEOUT(attempted, 10000);
        SETTLE(window);
        QVERIFY2(diagnostic_ready, "Publish the recovery error before admitting a retry");
        QCOMPARE(closed.count(), 1);
        QVERIFY(window.prepare_close());
        QVERIFY(!window.is_confirmation_pending());
        QCOMPARE(fixture.state()[0]["scale"].get<int>(), 1);
        QVERIFY(!QFile::exists(fixture.dir.filePath("overlap")));
    }

    void reentrant_refresh_data() {
        QTest::addColumn<bool>("fail");
        QTest::newRow("success") << false;
        QTest::newRow("error") << true;
    }

    void reentrant_refresh() {
        QFETCH(bool, fail);
        Fixture fixture;
        MainWindow window;
        SETTLE(window);
        auto * monitor = qobject_cast<MonitorProperties *>(window.get_monitors()[0]);
        monitor->set_scale(2);
        window.apply();
        SETTLE(window);
        fixture.hold_command("query");
        window.confirm_apply();
        QTRY_VERIFY_WITH_TIMEOUT(fixture.command_entered("query"), 10000);
        // This Cancel belongs to the finishing refresh, never to the next Apply.
        window.cancel_apply();
        if (fail) {
            fixture.write("query_failure", "1");
        }
        bool attempted = false;
        bool diagnostic_ready = false;
        connect(&window, &MainWindow::confirmation_pending_changed, &window, [&] {
            if (!attempted && window.is_direct_editing_allowed()) {
                attempted = true;
                diagnostic_ready = window.get_apply_error().contains("Could not refresh");
                QFile::remove(fixture.dir.filePath("query_failure"));
                monitor->set_scale(3);
                window.apply();
            }
        });
        fixture.release_command("query");
        QTRY_VERIFY_WITH_TIMEOUT(attempted, 10000);
        SETTLE(window);
        QCOMPARE(diagnostic_ready, fail);
        QVERIFY(window.is_confirmation_allowed());
        QVERIFY(window.get_apply_error().isEmpty());
        QCOMPARE(monitor->get_scale(), 3.0F);
        QCOMPARE(fixture.state()[0]["scale"].get<int>(), 3);
        window.cancel_apply();
        SETTLE(window);
        QCOMPARE(fixture.state()[0]["scale"].get<int>(), 2);
        QVERIFY(!QFile::exists(fixture.dir.filePath("overlap")));
    }

    void locked_transaction_data() {
        QTest::addColumn<QString>("action");
        for (const auto * action :
             { "confirm", "cancel", "timeout", "failed-rollback", "failed-refresh" }) {
            QTest::newRow(action) << QString::fromUtf8(action);
        }
    }

    void locked_transaction() {
        QFETCH(QString, action);
        Fixture fixture;
        fixture.multi_monitor();
        MainWindow window;
        SETTLE(window);
        auto * monitor = qobject_cast<MonitorProperties *>(window.get_monitors()[0]);
        QVERIFY(monitor != nullptr);
        monitor->set_scale(1.1F);
        monitor->set_active_resolution_index(1);
        monitor->set_position_x(240);
        monitor->set_position_y(120);
        const auto applied_draft = draft_state(*monitor);
        int unlocked = 0;
        // A permitted edit made synchronously on unlock must survive the older result.
        connect(&window, &MainWindow::confirmation_pending_changed, &window, [&] {
            if (window.is_direct_editing_allowed()) {
                ++unlocked;
                monitor->set_scale(1.2F);
            }
        });
        fixture.hold_command("mutation");
        window.apply();
        QTRY_VERIFY_WITH_TIMEOUT(fixture.command_entered("mutation"), 10000);
        verify_edits_locked(window, *monitor);
        QCOMPARE(draft_state(*monitor), applied_draft);
        QCOMPARE(fixture.state()[0]["scale"].get<double>(), 1.1);
        fixture.release_command("mutation");
        SETTLE(window);
        QVERIFY(window.is_confirmation_allowed());
        verify_edits_locked(window, *monitor);
        QCOMPARE(unlocked, 0);

        double live_scale = 1;
        if (action == "confirm" || action == "failed-refresh") {
            live_scale = 1.1;
            fixture.hold_command("query");
            window.confirm_apply();
            QTRY_VERIFY_WITH_TIMEOUT(fixture.command_entered("query"), 10000);
            QVERIFY(window.is_busy());
            QVERIFY(!window.is_confirmation_pending());
            verify_edits_locked(window, *monitor);
            if (action == "failed-refresh") {
                fixture.write("query_failure", "1");
            }
            fixture.release_command("query");
        } else {
            fixture.hold_command("mutation");
            if (action == "failed-rollback") {
                fixture.write("failures", "1");
            }
            if (action == "timeout") {
                QVERIFY(QMetaObject::invokeMethod(&window, "on_confirmation_timeout"));
            } else {
                window.cancel_apply();
            }
            QTRY_VERIFY_WITH_TIMEOUT(fixture.command_entered("mutation"), 10000);
            verify_edits_locked(window, *monitor);
            QCOMPARE(fixture.state()[0]["scale"].get<double>(), live_scale);
            fixture.release_command("mutation");
            if (action == "failed-rollback") {
                SETTLE(window);
                QVERIFY(window.is_confirmation_pending());
                QVERIFY(!window.is_confirmation_allowed());
                QVERIFY(window.get_apply_error().contains("Restore failed"));
                verify_edits_locked(window, *monitor);
                QCOMPARE(unlocked, 0);
                window.cancel_apply();
            }
        }
        SETTLE(window);
        QVERIFY(!window.is_confirmation_pending());
        QVERIFY(window.is_direct_editing_allowed());
        QCOMPARE(unlocked, 1);
        QCOMPARE(monitor->get_scale(), 1.2F);
        QCOMPARE(fixture.state()[0]["scale"].get<double>(), live_scale);
        if (action == "failed-refresh") {
            QVERIFY(window.get_apply_error().contains("Could not refresh monitors"));
        }
        QVERIFY(!QFile::exists(fixture.dir.filePath("overlap")));
    }

    void close_and_timeout() {
        Fixture f;
        auto initial_state = f.state();
        initial_state[0]["scale"] = 2;
        f.write("state", initial_state.dump());

        MainWindow window;
        SETTLE(window);
        window.apply();
        SETTLE(window);
        QVERIFY2(!window.prepare_close(), "Close waits asynchronously for restoration");
        SETTLE(window);
        QVERIFY2(
            window.prepare_close() && !window.is_confirmation_pending(), "Close restores state"
        );
        window.apply();
        SETTLE(window);
        QVERIFY2(
            QMetaObject::invokeMethod(&window, "on_confirmation_timeout", Qt::DirectConnection),
            "Timeout slot available"
        );
        SETTLE(window);
        QVERIFY2(
            !window.is_confirmation_pending() && window.get_confirmation_seconds_left() == 0,
            "Successful timeout resolves UI state"
        );
    }

    void timer_failure_and_close_retry() {
        Fixture f;
        auto initial_state = f.state();
        initial_state[0]["scale"] = 2;
        f.write("state", initial_state.dump());

        MainWindow window;
        SETTLE(window);
        window.apply();
        SETTLE(window);
        f.write("failures", "1");
        // Exercise actual timer delivery without waiting fifteen seconds.
        for (auto * timer : window.findChildren<QTimer *>()) {
            if (timer->isSingleShot() && timer->interval() == 15000) {
                timer->start(10);
            }
        }
        QTRY_VERIFY_WITH_TIMEOUT(!window.is_confirmation_allowed(), 20000);
        SETTLE(window);
        QVERIFY2(
            window.is_confirmation_pending() && !window.is_confirmation_allowed(),
            "Timeout failure keeps recovery UI"
        );
        QVERIFY2(
            window.get_confirmation_seconds_left() == 0 && !window.get_apply_error().isEmpty(),
            "Failed timeout stops countdown and reports error"
        );
        const auto calls = f.read("calls");
        QVERIFY2(
            QMetaObject::invokeMethod(&window, "on_countdown_tick", Qt::DirectConnection),
            "Tick slot available"
        );
        window.confirm_apply();
        SETTLE(window);
        window.apply();
        SETTLE(window);
        QVERIFY2(
            f.read("calls") == calls && window.is_confirmation_pending(), "No retry loop or bypass"
        );
        f.write("failures", "1");
        QVERIFY2(
            !window.prepare_close() && window.is_confirmation_pending(),
            "Close rejected on rollback failure"
        );
        SETTLE(window);
        QVERIFY2(!window.prepare_close(), "Retry close waits for rollback");
        SETTLE(window);
        QVERIFY2(
            window.prepare_close() && !window.is_confirmation_pending(),
            "Close restores before accepting"
        );
        QVERIFY2(window.prepare_close(), "Repeated close is harmless");
    }

    void disconnect_retry() {
        Fixture f;
        auto initial_state = f.state();
        initial_state[0]["scale"] = 2;
        f.write("state", initial_state.dump());

        MainWindow window;
        SETTLE(window);
        window.apply();
        SETTLE(window);
        f.write("state", "[]");
        QVERIFY2(
            QMetaObject::invokeMethod(&window, "on_confirmation_timeout", Qt::DirectConnection),
            "Timeout slot available"
        );
        SETTLE(window);
        QVERIFY2(
            window.is_confirmation_pending() && !window.is_confirmation_allowed(),
            "Disconnect does not crash or resolve pending state"
        );
        f.reset();
        window.cancel_apply();
        SETTLE(window);
        QVERIFY2(!window.is_confirmation_pending(), "Reconnect and retry from UI");
    }

    void apply_failure_recovery() {
        Fixture f;
        auto initial_state = f.state();
        initial_state[0]["scale"] = 2;
        f.write("state", initial_state.dump());

        MainWindow window;
        SETTLE(window);
        f.write("failures", "1");
        window.apply();
        SETTLE(window);
        QVERIFY2(
            !window.is_confirmation_pending() && !window.get_apply_error().isEmpty(),
            "Recovered Apply failure remains visible"
        );
        QVERIFY(window.is_direct_editing_allowed());
        f.write("failures", "2");
        window.apply();
        SETTLE(window);
        QVERIFY2(
            window.is_confirmation_pending() && !window.is_confirmation_allowed() &&
                window.get_confirmation_seconds_left() == 0 && !window.get_apply_error().isEmpty(),
            "Failed Apply and rollback show recovery UI"
        );
        QVERIFY(!window.is_direct_editing_allowed());
        window.cancel_apply();
        SETTLE(window);
        QVERIFY2(!window.is_confirmation_pending(), "UI can retry failed Apply recovery");
        QVERIFY(window.is_direct_editing_allowed());
    }

    void confirmed_refresh_failure() {
        Fixture f;
        auto initial_state = f.state();
        initial_state[0]["scale"] = 2;
        f.write("state", initial_state.dump());

        MainWindow window;
        SETTLE(window);
        window.apply();
        SETTLE(window);
        f.write("query_failure", "1");
        window.confirm_apply();
        SETTLE(window);
        QVERIFY2(
            !window.is_confirmation_pending() && !window.get_apply_error().isEmpty(),
            "Refresh failure is reported without reviving a confirmed transaction"
        );
        f.reset();
    }

    void hung_apply() {
        Fixture f;

        f.reset();
        MainWindow window;
        SETTLE(window);
        auto * monitor = qobject_cast<MonitorProperties *>(window.get_monitors()[0]);
        monitor->set_scale(2);
        int ticks = 0;
        QTimer timer;
        QObject::connect(&timer, &QTimer::timeout, [&] { ++ticks; });
        timer.start(10);
        f.write("hangs", "1");
        window.apply();
        // Conflicts are rejected while the snapshot query/Apply is in flight.
        window.apply();
        window.confirm_apply();
        window.set_backend_mode(2);
        QVERIFY2(
            window.get_backend_mode() == 0 && !window.is_confirmation_allowed(), "In-flight gate"
        );
        window.cancel_apply();  // Remember intent; never run rollback concurrently.
        QVERIFY2(!window.prepare_close(), "Close must wait for in-flight Apply and recovery");
        SETTLE(window);
        QVERIFY2(ticks > 100, "Hung Apply blocked Qt event loop");
        QVERIFY2(
            f.state()[0]["scale"] == 1 && !window.is_confirmation_pending(),
            "Timed out partial Apply was not restored"
        );
        QVERIFY2(window.get_apply_error().contains("timeout"), "Apply timeout missing diagnostic");
        QVERIFY(!QFile::exists(f.dir.filePath("overlap")));
        const auto calls = f.read("calls");
        const auto apply = calls.find("2.000000");
        const auto restore = calls.find("1.000000", apply);
        QVERIFY(apply != std::string::npos);
        QVERIFY(restore != std::string::npos && restore > apply);
        QVERIFY(calls.find("2.000000", restore) == std::string::npos);
    }

    void hung_rollback() {
        Fixture f;

        MainWindow window;
        SETTLE(window);
        auto * monitor = qobject_cast<MonitorProperties *>(window.get_monitors()[0]);
        monitor->set_scale(2);
        f.reset();
        f.write("hangs", "2");
        window.apply();
        SETTLE(window);
        QVERIFY2(
            window.is_confirmation_pending() && !window.is_confirmation_allowed(),
            "Timed out rollback must retain snapshot"
        );
        QVERIFY2(
            window.get_confirmation_seconds_left() == 0,
            "Failed Apply started confirmation countdown"
        );
        window.cancel_apply();
        SETTLE(window);
        QVERIFY2(
            !window.is_confirmation_pending() && f.state()[0]["scale"] == 1,
            "Manual retry after timeout"
        );
    }

    void destroyed_window() {
        Fixture f;

        f.reset();
        auto window = std::make_unique<MainWindow>();
        SETTLE(*window);
        auto * monitor = qobject_cast<MonitorProperties *>(window->get_monitors()[0]);
        monitor->set_scale(2);
        window->apply();
        window.reset();
        drain_jobs();
        QVERIFY2(f.state()[0]["scale"] == 1, "Destroyed window lost its recovery snapshot");
    }
};
QTEST_MAIN(MainWindowTests)
#include "main_window.moc"
