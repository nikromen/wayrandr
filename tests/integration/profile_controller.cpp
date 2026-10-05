#include <qfiledevice.h>
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
#include <QPoint>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "backend/auto_wlr_randr/profile_backend.hpp"
#include "backend/kanshi/profile_backend.hpp"
#include "backend/profile/editor_backend.hpp"
#include "backend/profile/types.hpp"
#include "models/main_window.hpp"
#include "support/environment.hpp"
#include "support/fixture.hpp"
#include "utils/helpers.hpp"

#define SETTLE(window) QTRY_VERIFY_WITH_TIMEOUT(!(window).is_busy(), 20000)

class ProfileControllerTests : public QObject {
    Q_OBJECT
private slots:

    void cleanupTestCase() { shutdown_commands(); }  // NOLINT: Qt Test hook name.

    void daemon_data() {
        QTest::addColumn<int>("mode");
        QTest::newRow("kanshi") << 1;
        QTest::newRow("auto-wlr-randr") << 2;
    }

    void snapshot_data() {
        QTest::addColumn<int>("mode");
        QTest::addColumn<int>("state_kind");
        for (const int mode : { 1, 2 }) {
            for (const int state_kind : { 0, 1, 2 }) {
                QTest::newRow(qPrintable(QString("%1-%2").arg(mode).arg(state_kind)))
                    << mode << state_kind;
            }
        }
    }

    void snapshot() {
        QFETCH(int, mode);
        QFETCH(int, state_kind);
        Fixture f;
        f.multi_monitor();
        auto monitors = f.state();
        monitors[0]["make"] = "Výrobce";
        monitors[0]["model"] = "Panel";
        monitors[0]["serial"] = "123";
        monitors[0]["position"] = { { "x", -1920 }, { "y", 25 } };
        monitors[0]["scale"] = 1.5;
        monitors[0]["transform"] = "90";
        monitors[0]["adaptive_sync"] = true;
        if (state_kind == 1) {
            monitors[0]["modes"] = json::array();
        } else if (state_kind == 2) {
            monitors = json::array();
        }
        f.write("state", monitors.dump());
        std::shared_ptr<ProfileEditorBackend> backend;
        if (mode == 1) {
            backend = std::make_shared<KanshiProfileBackend>();
        } else {
            backend = std::make_shared<AutoWlrRandrProfileBackend>();
        }
        const auto captured = std::make_shared<profile::ProfileDefinition>();
        const auto result = std::make_shared<JobResult>();
        run_job(
            this,
            [backend, captured, result] {
                auto snapshot = backend->create_profile_from_live("live");
                return [captured, result, snapshot = std::move(snapshot)] {
                    *captured = snapshot;
                    result->done = true;
                };
            },
            [result](const std::string & error) {
                result->error = error;
                result->done = true;
            }
        );
        QTRY_VERIFY(result->done);
        QVERIFY2(result->error.empty(), result->error.c_str());
        QCOMPARE(captured->id, std::string("live"));
        QCOMPARE(captured->outputs.size(), monitors.size());
        if (state_kind == 2) {
            return;
        }
        const auto & enabled = captured->outputs[0];
        QCOMPARE(enabled.output, std::string("Výrobce Panel 123"));
        QVERIFY(enabled.enabled == true);
        if (state_kind == 1) {
            QVERIFY(!enabled.mode.has_value());
        } else {
            QVERIFY(enabled.mode == "1920x1080@60.000000Hz");
        }
        QVERIFY(enabled.pos == "-1920,25");
        QVERIFY(enabled.scale == 1.5F);
        QVERIFY(enabled.transform == "90");
        QVERIFY(enabled.adaptive_sync == true);
        const auto & disabled = captured->outputs[1];
        QCOMPARE(disabled.output, std::string("DP-2"));
        QVERIFY(disabled.enabled == false);
        QVERIFY(
            !disabled.mode && !disabled.pos && !disabled.scale && !disabled.transform &&
            !disabled.adaptive_sync
        );
    }

    void daemon() {
        QFETCH(int, mode);
        Fixture f;

        f.reset();
        auto monitor_state = f.state();
        monitor_state[0]["name"] = "DP[1";
        monitor_state[0]["position"] = { { "x", 80 }, { "y", 25 } };
        f.write("state", monitor_state.dump());
        f.write("daemon_mode", "success");
        f.write("daemon_calls", "");
        qputenv("WAYLAND_DISPLAY", "test-display");
        f.write("fr.emersion.kanshi.test-display", "");
        QVERIFY2(QDir().mkpath(f.dir.filePath("auto-wlr-randr")), "Fake socket directory");
        f.write("auto-wlr-randr/auto-wlr-randr.sock", "");
        QVERIFY2(QDir().mkpath(f.dir.filePath("bin")), "Fake binary directory");
        qputenv("PATH", (f.dir.filePath("bin") + ":" + f.dir.path()).toUtf8());
        for (const char * name : { "kanshi", "kanshictl", "auto-wlr-randr", "auto-wlr-randrctl" }) {
            QVERIFY2(
                QFile::copy(FAKE_DAEMON, f.dir.filePath(QString("bin/%1").arg(name))),
                "Fake daemon copy"
            );
            QVERIFY2(
                QFile::setPermissions(
                    f.dir.filePath(QString("bin/%1").arg(name)),
                    QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner
                ),
                "Fake daemon permission"
            );
        }
        MainWindow window;
        SETTLE(window);
        auto * editor = window.get_profile_editor();

        window.set_backend_mode(mode);
        SETTLE(window);
        QVERIFY2(
            editor->is_daemon_running() && editor->get_active_profile_id() == "test",
            "Daemon status delivery"
        );
        f.write("calls", "");
        editor->refresh_connected_outputs();
        SETTLE(window);
        int expected_queries = 1;
        if (mode == 1) {
            ++expected_queries;  // Kanshi status reads monitor names separately.
        }
        QCOMPARE(QString::fromStdString(f.read("calls")).count("[\"--json\"]"), expected_queries);
        QCOMPARE(editor->get_live_position_for_output("DP*"), QPoint(80, 25));
        QCOMPARE(editor->get_live_position_for_output("DP[1"), QPoint(0, 0));
        f.write("daemon_mode", "exit");
        editor->refresh_daemon_status();
        window.set_backend_mode(0);
        QVERIFY2(window.get_backend_mode() == mode, "Backend changed during queued status request");
        SETTLE(window);
        QVERIFY2(
            !editor->is_daemon_running() &&
                editor->get_daemon_status_text().contains("exit code 17") &&
                editor->get_daemon_status_text().contains("daemon failure detail"),
            "Daemon failure was swallowed"
        );
        f.write("daemon_mode", "success");
        std::vector<std::string> invalid_replies{ "", "unrecognized status\n" };
        std::string no_profile = "Current profile: (none)\n";
        QString no_profile_id = "(none)";
        if (mode == 2) {
            invalid_replies = { R"({"connected_outputs":[]})",
                                R"({"active_profile":"None"})",
                                R"({"active_profile":"None","connected_outputs":{}})" };
            no_profile = R"({"active_profile":"None","connected_outputs":[]})";
            no_profile_id = "None";
        }
        for (const auto & reply : invalid_replies) {
            f.write("status_reply", reply);
            editor->refresh_daemon_status();
            SETTLE(window);
            QVERIFY(!editor->is_daemon_running());
            QVERIFY(editor->get_active_profile_id().isEmpty());
            QVERIFY(editor->get_daemon_status_text().contains("Daemon status failed"));
        }
        f.write("status_reply", no_profile);
        editor->refresh_daemon_status();
        SETTLE(window);
        QVERIFY(editor->is_daemon_running());
        QCOMPARE(editor->get_active_profile_id(), no_profile_id);
        QVERIFY(QFile::remove(f.dir.filePath("status_reply")));
        editor->create_profile_from_live(QString("test-%1").arg(mode));
        SETTLE(window);
        editor->switch_profile(mode == 2);
        SETTLE(window);
        QVERIFY2(editor->is_daemon_running(), "Profile switch/status failed");
        QCOMPARE(editor->get_active_profile_id(), QString("test-%1").arg(mode));
        f.write("daemon_mode", "delay");
        editor->save_profile();
        editor->add_exec_command("test-command");  // Simulate a programmatic edit in flight.
        SETTLE(window);
        QVERIFY2(
            editor->is_dirty() && editor->get_exec_commands().contains("test-command"),
            "Late save discarded newer edits"
        );
        QVERIFY2(
            editor->get_save_status_text().contains("Newer edits remain unsaved"),
            qPrintable(editor->get_save_status_text())
        );
        f.write("daemon_mode", "success");
        editor->save_profile();
        SETTLE(window);
        QVERIFY2(!editor->is_dirty(), qPrintable(editor->get_save_status_text()));
        QVERIFY(editor->get_exec_commands().contains("test-command"));
        window.set_backend_mode(0);
        editor->discard_changes();

        const auto calls = f.read("daemon_calls");
        if (mode == 1) {
            QVERIFY2(
                calls.find(R"(["kanshictl", "switch", "test-1"])") != std::string::npos,
                "Kanshi switch arguments"
            );
        } else {
            QVERIFY2(
                calls.find(R"(["auto-wlr-randrctl", "switch", "--force", "--", "test-2"])") !=
                    std::string::npos,
                "Auto switch arguments"
            );
        }
    }
};
QTEST_MAIN(ProfileControllerTests)
#include "profile_controller.moc"
