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
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <string>

#include "models/main_window.hpp"
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

    void daemon() {
        QFETCH(int, mode);
        Fixture f;

        f.reset();
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
