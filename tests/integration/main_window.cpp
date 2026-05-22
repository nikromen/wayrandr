#include "models/main_window.hpp"

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
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QVariant>
#include <cstddef>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

#include "models/monitor_properties.hpp"
#include "support/environment.hpp"
#include "support/fixture.hpp"
#include "utils/helpers.hpp"

#define SETTLE(window) QTRY_VERIFY_WITH_TIMEOUT(!(window).is_busy(), 20000)

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
        f.write("failures", "2");
        window.apply();
        SETTLE(window);
        QVERIFY2(
            window.is_confirmation_pending() && !window.is_confirmation_allowed() &&
                window.get_confirmation_seconds_left() == 0 && !window.get_apply_error().isEmpty(),
            "Failed Apply and rollback show recovery UI"
        );
        window.cancel_apply();
        SETTLE(window);
        QVERIFY2(!window.is_confirmation_pending(), "UI can retry failed Apply recovery");
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
