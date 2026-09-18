#include "models/main_window.hpp"

#include <qcontainerfwd.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qtenvironmentvariables.h>
#include <qtestcase.h>
#include <qtmetamacros.h>
#include <signal.h>  // NOLINT: POSIX kill() verifies fake process cleanup.

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

#include "backend/base.hpp"
#include "models/monitor_block.hpp"
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

    void refreshed_output_set_data() {
        QTest::addColumn<bool>("confirm");
        QTest::newRow("confirm") << true;
        QTest::newRow("cancel") << false;
    }

    void refreshed_output_set() {
        QFETCH(bool, confirm);
        Fixture fixture;
        MainWindow window;
        SETTLE(window);
        auto * first = qobject_cast<MonitorProperties *>(window.get_monitors()[0]);
        first->set_scale(2);
        window.apply();
        SETTLE(window);
        auto state = fixture.state();
        auto second = state[0];
        second["name"] = "DP-NEW";
        second["description"] = "New monitor";
        second["scale"] = 1.5;
        state.insert(state.begin(), second);
        fixture.write("state", state.dump());
        if (confirm) {
            window.confirm_apply();
        } else {
            window.cancel_apply();
        }
        SETTLE(window);
        QCOMPARE(window.get_monitors().size(), 2);
        auto * added = qobject_cast<MonitorProperties *>(window.get_monitors()[0]);
        QCOMPARE(added->get_name(), "DP-NEW");
        QCOMPARE(added->get_description(), "New monitor");
        QCOMPARE(added->get_scale(), 1.5F);
        QCOMPARE(window.get_monitors()[1], first);
        QVERIFY(window.get_monitor_block("DP-NEW") != nullptr);
        int expected_scale = 1;
        if (confirm) {
            expected_scale = 2;
        }
        QCOMPARE(first->get_scale(), static_cast<float>(expected_scale));
        QVERIFY(!QFile::exists(fixture.dir.filePath("overlap")));
    }

    void return_to_direct_data() {
        QTest::addColumn<int>("mode");
        QTest::newRow("kanshi") << static_cast<int>(editor_mode::KANSHI);
        QTest::newRow("auto-wlr-randr") << static_cast<int>(editor_mode::AUTO_WLR_RANDR);
    }

    void return_to_direct() {
        QFETCH(int, mode);
        Fixture fixture;
        fixture.profiles();
        MainWindow window;
        SETTLE(window);
        auto * first = qobject_cast<MonitorProperties *>(window.get_monitors()[0]);
        bool gate_closed_at_notify = false;
        connect(&window, &MainWindow::backend_mode_changed, &window, [&] {
            if (window.get_backend_mode() == editor_mode::WLR_RANDR) {
                gate_closed_at_notify = window.is_busy();
                window.apply();
            }
        });
        window.set_backend_mode(mode);
        SETTLE(window);
        auto state = fixture.state();
        state[0]["scale"] = 2;
        state[0]["transform"] = "90";
        state[0]["position"] = { { "x", -240 }, { "y", 80 } };
        state[0]["description"] = "Current monitor";
        fixture.write("state", state.dump());
        window.set_backend_mode(editor_mode::WLR_RANDR);
        SETTLE(window);
        QVERIFY(gate_closed_at_notify);
        QVERIFY(!window.is_confirmation_pending());
        QCOMPARE(first->get_scale(), 2.0F);
        QCOMPARE(first->get_transform(), "90");
        QCOMPARE(first->get_position_x(), -240);
        QCOMPARE(first->get_description(), "Current monitor");
        QCOMPARE(window.get_monitors()[0], first);
    }

    void output_identity_and_draft_data() { return_to_direct_data(); }

    void output_identity_and_draft() {
        QFETCH(int, mode);
        Fixture fixture;
        fixture.profiles();
        fixture.multi_monitor();
        auto initial = fixture.state();
        initial[1]["enabled"] = true;
        fixture.write("state", initial.dump());
        MainWindow window;
        SETTLE(window);
        auto * first = qobject_cast<MonitorProperties *>(window.get_monitors()[0]);
        auto * second = qobject_cast<MonitorProperties *>(window.get_monitors()[1]);
        auto * first_capture = window.get_monitor_block("DP-1");
        first->set_scale(1.3F);
        first->set_active_resolution_index(1);
        first->set_transform("flipped-90");
        first->set_position_x(-320);
        first->set_position_y(80);
        first->set_adaptive_sync(true);
        first->set_enabled(false);
        first->set_enabled(true);
        const auto draft = draft_state(*first);
        QVERIFY(first->is_dirty());
        window.set_backend_mode(mode);
        SETTLE(window);
        auto state = initial;
        state[1]["scale"] = 3;
        auto added = state[1];
        added["name"] = "DP-NEW";
        added["scale"] = 1.7;
        state = json::array({ state[1], added });
        fixture.write("state", state.dump());
        fixture.hold_command("query");
        window.set_backend_mode(editor_mode::WLR_RANDR);
        QTRY_VERIFY_WITH_TIMEOUT(fixture.command_entered("query"), 10000);
        verify_edits_locked(window, *first);
        window.apply();
        window.set_backend_mode(mode);
        QCOMPARE(window.get_backend_mode(), editor_mode::WLR_RANDR);
        fixture.release_command("query");
        SETTLE(window);
        QCOMPARE(window.get_monitors().size(), 3);
        QCOMPARE(window.get_monitors()[0], second);
        QCOMPARE(second->get_scale(), 3.0F);
        auto * new_model = qobject_cast<MonitorProperties *>(window.get_monitors()[1]);
        QCOMPARE(new_model->get_name(), "DP-NEW");
        QCOMPARE(new_model->get_scale(), 1.7F);
        QCOMPARE(window.get_monitors()[2], first);
        QVERIFY(!first->is_connected());
        QVERIFY(!first->is_enabled());
        QVERIFY(first->is_dirty());
        QCOMPARE(first->get_layout_width(), 0);
        QCOMPARE(first->get_specs().get_enabled_monitor_settings()->get_scale(), 1.3F);
        first->set_enabled(true);
        first->set_scale(4);
        QVERIFY(!first->is_enabled());
        QCOMPARE(first->get_specs().get_enabled_monitor_settings()->get_scale(), 1.3F);
        QCOMPARE(window.get_monitor_block("DP-1"), first_capture);
        for (auto * timer : first_capture->findChildren<QTimer *>()) {
            QVERIFY(!timer->isActive());
        }
        // The disconnected draft is excluded from Apply and survives another transaction.
        window.apply();
        SETTLE(window);
        window.confirm_apply();
        SETTLE(window);
        QVERIFY(first->is_dirty());
        QVERIFY(fixture.read("calls").find("\"DP-1\"") == std::string::npos);

        window.set_backend_mode(mode);
        SETTLE(window);
        auto reconnected = initial[0];
        reconnected["scale"] = 2.2;
        reconnected["description"] = "Reconnected monitor";
        auto modes = reconnected["modes"];
        reconnected["modes"] = json::array({ modes[1], modes[0] });
        state = json::array({ reconnected, added, state[0] });
        fixture.write("state", state.dump());
        window.set_backend_mode(editor_mode::WLR_RANDR);
        SETTLE(window);
        QCOMPARE(window.get_monitors()[0], first);
        QCOMPARE(window.get_monitors()[1], new_model);
        QCOMPARE(window.get_monitors()[2], second);
        QVERIFY(first->is_connected());
        QCOMPARE(draft_state(*first), draft);
        QCOMPARE(fixture.state()[0]["scale"].get<double>(), 2.2);
        QCOMPARE(first->get_resolution_width(), 1280);
        window.apply();
        SETTLE(window);
        QVERIFY(!first->is_dirty());
        QCOMPARE(fixture.state()[0]["scale"].get<double>(), 1.3);
        window.confirm_apply();
        SETTLE(window);
        QCOMPARE(first->get_description(), "Reconnected monitor");
        QCOMPARE(first->get_active_resolution_index(), size_t{ 0 });
        QCOMPARE(first->get_resolution_width(), 1280);
        QVERIFY(!QFile::exists(fixture.dir.filePath("overlap")));
    }

    void return_refresh_error_data() {
        QTest::addColumn<QString>("error");
        for (const auto * error : { "exit", "non-array", "duplicate", "partial-decode" }) {
            QTest::newRow(error) << QString::fromUtf8(error);
        }
    }

    void return_refresh_error() {
        QFETCH(QString, error);
        Fixture fixture;
        fixture.profiles();
        fixture.multi_monitor();
        auto original = fixture.state();
        original[1]["enabled"] = true;
        fixture.write("state", original.dump());
        MainWindow window;
        SETTLE(window);
        const auto models = window.get_monitors();
        auto * first = qobject_cast<MonitorProperties *>(models[0]);
        auto * second = qobject_cast<MonitorProperties *>(models[1]);
        first->set_scale(1.4F);
        window.set_backend_mode(editor_mode::KANSHI);
        SETTLE(window);
        auto state = original;
        state[1]["scale"] = 3;
        if (error == "exit") {
            fixture.write("query_failure", "1");
        } else if (error == "duplicate") {
            state[1]["name"] = "DP-1";
        } else if (error == "partial-decode") {
            state[1].erase("scale");
        }
        if (error == "non-array") {
            fixture.write("state", "{}");
        } else {
            fixture.write("state", state.dump());
        }
        fixture.hold_command("query");
        window.set_backend_mode(editor_mode::WLR_RANDR);
        QTRY_VERIFY_WITH_TIMEOUT(fixture.command_entered("query"), 10000);
        verify_edits_locked(window, *first);
        window.set_backend_mode(editor_mode::AUTO_WLR_RANDR);
        QCOMPARE(window.get_backend_mode(), editor_mode::WLR_RANDR);
        fixture.release_command("query");
        SETTLE(window);
        QCOMPARE(window.get_monitors(), models);
        QVERIFY(window.get_apply_error().contains("Could not refresh monitors"));
        QVERIFY(first->is_connected());
        QVERIFY(second->is_connected());
        QCOMPARE(first->get_scale(), 1.4F);
        QCOMPARE(second->get_scale(), 1.0F);
        QVERIFY(window.is_direct_editing_allowed());
        first->set_scale(1.5F);
        // A later permitted edit made on unlock must survive the old query's completion.
        fixture.write("state", original.dump());
        QFile::remove(fixture.dir.filePath("query_failure"));
        window.set_backend_mode(editor_mode::KANSHI);
        SETTLE(window);
        original[1]["scale"] = 3;
        fixture.write("state", original.dump());
        fixture.hold_command("query");
        window.set_backend_mode(editor_mode::WLR_RANDR);
        QTRY_VERIFY_WITH_TIMEOUT(fixture.command_entered("query"), 10000);
        verify_edits_locked(window, *first);
        bool edited = false;
        connect(&window, &MainWindow::confirmation_pending_changed, &window, [&] {
            if (window.is_direct_editing_allowed()) {
                edited = true;
                first->set_scale(1.6F);
            }
        });
        fixture.release_command("query");
        SETTLE(window);
        QVERIFY(edited);
        QCOMPARE(first->get_scale(), 1.6F);
        QCOMPARE(second->get_scale(), 3.0F);
        QCOMPARE(window.get_monitors(), models);
        QVERIFY(window.get_apply_error().isEmpty());
        QCOMPARE(fixture.state()[0]["scale"].get<int>(), 1);
        QVERIFY(!QFile::exists(fixture.dir.filePath("overlap")));
    }

    void disconnect_during_capture() {
        Fixture fixture;
        fixture.profiles();
        fixture.install(FAKE_GRIM, "grim");
        fixture.write("grim_mode", "success");
        const auto original = fixture.state();
        MainWindow window;
        SETTLE(window);
        auto * monitor = qobject_cast<MonitorProperties *>(window.get_monitors()[0]);
        auto * block = qobject_cast<MonitorBlock *>(window.get_monitor_block("DP-1"));
        QVERIFY(block != nullptr);
        block->stop_capture();
        drain_jobs();
        const auto image_before = block->get_screen_image();
        QSignalSpy images(block, &MonitorBlock::screen_image_changed);
        window.set_backend_mode(editor_mode::KANSHI);
        SETTLE(window);
        fixture.write("state", "[]");
        fixture.hold_command("query");
        window.set_backend_mode(editor_mode::WLR_RANDR);
        QTRY_VERIFY_WITH_TIMEOUT(fixture.command_entered("query"), 10000);
        // Accepted capture is queued behind the query. Its completion arrives after
        // reconciliation, whether its process starts before or after GUI delivery.
        block->start_capture();
        block->capture_now();
        fixture.release_command("query");
        SETTLE(window);
        QVERIFY(!monitor->is_connected());
        drain_jobs();
        QCOMPARE(images.count(), 0);
        QCOMPARE(block->get_screen_image(), image_before);
        if (QFile::exists(fixture.dir.filePath("grim_pid"))) {
            QCOMPARE(::kill(std::stoi(fixture.read("grim_pid")), 0), -1);
        }
        block->capture_now();
        drain_jobs();
        QCOMPARE(images.count(), 0);
        fixture.write("grim_mode", "success");
        window.set_backend_mode(editor_mode::KANSHI);
        SETTLE(window);
        fixture.write("state", original.dump());
        window.set_backend_mode(editor_mode::WLR_RANDR);
        SETTLE(window);
        QCOMPARE(window.get_monitors()[0], monitor);
        QCOMPARE(window.get_monitor_block("DP-1"), block);
        block->capture_now();
        drain_jobs();
        QCOMPARE(images.count(), 1);
        QVERIFY(!block->get_screen_image().isEmpty());
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
            !window.can_switch_backend(editor_mode::KANSHI) &&
                !window.can_switch_backend(editor_mode::AUTO_WLR_RANDR),
            "UI blocks switching away"
        );
        window.set_backend_mode(editor_mode::AUTO_WLR_RANDR);
        window.apply();
        SETTLE(window);
        QVERIFY2(
            window.get_backend_mode() == editor_mode::WLR_RANDR, "Pending backend remains active"
        );
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
        window.set_backend_mode(editor_mode::AUTO_WLR_RANDR);
        QVERIFY2(
            window.get_backend_mode() == editor_mode::WLR_RANDR &&
                !window.is_confirmation_allowed(),
            "In-flight gate"
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
