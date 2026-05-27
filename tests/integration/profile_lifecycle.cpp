#include <qobject.h>
#include <qtenvironmentvariables.h>
#include <qtestcase.h>
#include <qtmetamacros.h>

#include <QDir>
#include <QFile>
#include <QString>
#include <QTest>
#include <cstddef>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "backend/auto_wlr_randr/config_repository.hpp"
#include "backend/auto_wlr_randr/profile_backend.hpp"
#include "backend/kanshi/config_repository.hpp"
#include "backend/kanshi/profile_backend.hpp"
#include "backend/profile/editor_backend.hpp"
#include "backend/profile/types.hpp"
#include "models/main_window.hpp"
#include "models/profile/profile_output_properties.hpp"
#include "support/fixture.hpp"
#include "utils/helpers.hpp"

#define SETTLE(window) QTRY_VERIFY_WITH_TIMEOUT(!(window).is_busy(), 20000)

namespace {
auto output(ProfileEditorController * editor) -> ProfileOutputProperties * {
    return qobject_cast<ProfileOutputProperties *>(editor->get_outputs().value(0));
}

auto config_name(int mode) -> const char * {
    if (mode == 1) {
        return "kanshi/config";
    }
    return "auto-wlr-randr/config.toml";
}
}  // namespace

class ProfileLifecycleTests : public QObject {
    Q_OBJECT
private slots:

    void cleanupTestCase() { shutdown_commands(); }  // NOLINT: Qt Test hook name.

    void lifecycle_data() {
        QTest::addColumn<int>("mode");
        QTest::newRow("kanshi") << 1;
        QTest::newRow("auto-wlr-randr") << 2;
    }

    void document_operations_data() { lifecycle_data(); }

    void document_operations() {
        QFETCH(int, mode);
        Fixture f;
        f.profiles();
        const KanshiProfileBackend kanshi;
        const AutoWlrRandrProfileBackend auto_wlr;
        const ProfileEditorBackend * backend = &auto_wlr;
        if (mode == 1) {
            backend = &kanshi;
        }
        auto document = backend->load_config();
        const auto original = document;
        auto replacement = document.profiles[0];
        replacement.exec = { "echo preserved" };
        replacement.anonymous = true;
        replacement.outputs[0].scale = 1.5F;
        backend->add_profile(document, replacement);
        QCOMPARE(document.profiles.size(), size_t(2));
        QCOMPARE(document.profiles[0].id, std::string("desk"));
        QCOMPARE(document.profiles[0].exec, replacement.exec);
        QCOMPARE(document.profiles[1].id, original.profiles[1].id);
        QVERIFY(document.profiles[1].outputs[0].enabled == false);
        backend->duplicate_profile(document, "desk", "copy");
        QCOMPARE(document.profiles.size(), size_t(3));
        QCOMPARE(document.profiles[2].id, std::string("copy"));
        QVERIFY(!document.profiles[2].anonymous);
        QCOMPARE(document.profiles[2].exec, replacement.exec);
        QVERIFY(document.profiles[2].outputs[0].scale == 1.5F);
        backend->duplicate_profile(document, "desk", "spare");
        QCOMPARE(document.profiles.size(), size_t(3));
        QCOMPARE(document.profiles[1].id, std::string("spare"));
        QCOMPARE(document.profiles[1].exec, replacement.exec);
        QVERIFY(!document.profiles[1].anonymous);
        QVERIFY_THROWS_EXCEPTION(
            std::invalid_argument, backend->duplicate_profile(document, "missing", "new")
        );
        backend->delete_profile(document, "missing");
        QCOMPARE(document.profiles.size(), size_t(3));
        backend->delete_profile(document, "spare");
        QCOMPARE(document.profiles.size(), size_t(2));
        QCOMPARE(document.profiles[1].id, std::string("copy"));
        QCOMPARE(document.path, original.path);
        QCOMPARE(document.includes, original.includes);
        QCOMPARE(document.on_no_match_exec, original.on_no_match_exec);
        QCOMPARE(document.global_outputs.size(), original.global_outputs.size());
        if (!original.global_outputs.empty()) {
            QCOMPARE(document.global_outputs[1].output, original.global_outputs[1].output);
            QVERIFY(document.global_outputs[1].alias == original.global_outputs[1].alias);
        }
        QCOMPARE(document.file_snapshot.target, original.file_snapshot.target);
        QVERIFY(document.file_snapshot.content == original.file_snapshot.content);
        QCOMPARE(document.file_snapshot.identity, original.file_snapshot.identity);
    }

    void lifecycle() {
        QFETCH(int, mode);
        Fixture f;
        f.profiles();
        {
            MainWindow window;
            SETTLE(window);
            window.set_backend_mode(mode);
            SETTLE(window);
            auto * editor = window.get_profile_editor();
            QVERIFY(editor->select_profile("desk"));
            QVERIFY(output(editor) != nullptr);
            output(editor)->set_scale(1.5F);
            output(editor)->set_mode("1280x720@75Hz");
            output(editor)->set_transform("90");
            output(editor)->set_adaptive_sync(true);
            editor->add_exec_command("notify-send saved");
            editor->save_profile();
            SETTLE(window);
            QVERIFY2(!editor->is_dirty(), qPrintable(editor->get_daemon_status_text()));
        }
        MainWindow window;
        SETTLE(window);
        window.set_backend_mode(mode);
        SETTLE(window);
        auto * editor = window.get_profile_editor();
        QVERIFY(editor->select_profile("desk"));
        QCOMPARE(output(editor)->get_scale(), 1.5F);
        QCOMPARE(output(editor)->get_mode(), QString("1280x720@75Hz"));
        QCOMPARE(output(editor)->get_transform(), QString("90"));
        QVERIFY(output(editor)->is_adaptive_sync());
        QVERIFY(editor->get_exec_commands().contains("notify-send saved"));
        QVERIFY(editor->select_profile("spare"));
        QVERIFY(!output(editor)->is_enabled());
        QVERIFY(editor->select_profile("desk"));
        editor->duplicate_profile("desk", "copy");
        editor->save_profile();
        SETTLE(window);
        editor->reload_config_from_disk();
        QCOMPARE(editor->get_selected_profile_id(), QString("copy"));
        QCOMPARE(output(editor)->get_scale(), 1.5F);
        editor->delete_profile("copy");
        SETTLE(window);
        editor->reload_config_from_disk();
        QVERIFY(!editor->get_profile_ids().contains("copy"));
        QCOMPARE(editor->get_profile_ids().size(), 2);
        if (mode == 1) {
            const auto config =
                KanshiConfigRepository(f.dir.filePath(config_name(mode)).toStdString()).load();
            QVERIFY(
                config.includes == std::vector<std::string>{ "$XDG_CONFIG_HOME/kanshi/extra.conf" }
            );
            QVERIFY(config.preserved_directives.empty());
            QCOMPARE(config.global_outputs.size(), size_t(2));
            QCOMPARE(config.global_outputs[1].criteria, std::string("Example Model Serial"));
            QVERIFY(config.global_outputs[1].alias == "$desk");
            QCOMPARE(config.global_outputs[0].criteria, std::string("HDMI-1"));
            QVERIFY(config.global_outputs[0].enabled == false);
        } else {
            const auto config =
                AutoWlrRandrConfigRepository(f.dir.filePath(config_name(mode)).toStdString())
                    .load();
            QVERIFY(config.on_no_match_exec == std::vector<std::string>{ "notify-send unmatched" });
            bool found = false;
            for (const auto & profile : config.profiles) {
                if (profile.id == "desk") {
                    found = true;
                    QVERIFY(profile.settings[0].right_of == "HDMI-1");
                    QVERIFY(!profile.settings[0].pos.has_value());
                }
            }
            QVERIFY(found);
        }
    }

    void move_relative_output() {
        Fixture f;
        f.profiles();
        MainWindow window;
        SETTLE(window);
        window.set_backend_mode(2);
        SETTLE(window);
        auto * editor = window.get_profile_editor();
        QVERIFY(editor->select_profile("desk"));
        output(editor)->set_position_x(600);
        output(editor)->set_position_y(120);
        editor->save_profile();
        SETTLE(window);
        QVERIFY(!editor->is_dirty());
        const auto config =
            AutoWlrRandrConfigRepository(f.dir.filePath("auto-wlr-randr/config.toml").toStdString())
                .load();
        bool found = false;
        for (const auto & profile : config.profiles) {
            if (profile.id == "desk") {
                found = true;
                QVERIFY(profile.settings[0].pos == "600,120");
                QVERIFY(!profile.settings[0].right_of.has_value());
            }
        }
        QVERIFY(found);
        editor->reload_config_from_disk();
        QCOMPARE(output(editor)->get_position_x(), 600);
        QCOMPARE(output(editor)->get_position_y(), 120);
    }

    void dirty_edits_data() { lifecycle_data(); }

    void dirty_edits() {
        QFETCH(int, mode);
        Fixture f;
        f.profiles();
        MainWindow window;
        SETTLE(window);
        window.set_backend_mode(mode);
        SETTLE(window);
        auto * editor = window.get_profile_editor();
        QVERIFY(editor->select_profile("desk"));
        output(editor)->set_scale(2);
        QVERIFY(!editor->select_profile("spare"));
        QCOMPARE(editor->get_selected_profile_id(), QString("desk"));
        QVERIFY(editor->is_dirty());
        QCOMPARE(output(editor)->get_scale(), 2.0F);
        editor->discard_changes();
        QCOMPARE(output(editor)->get_scale(), 1.0F);
        QVERIFY(!editor->is_dirty());
        QVERIFY(editor->select_profile("spare"));
        QVERIFY(editor->select_profile("desk"));
        output(editor)->set_scale(2);
        const auto * const name = config_name(mode);
        const auto original = f.read(name);
        f.write(name, "profile broken {");
        editor->reload_config_from_disk();
        QVERIFY(editor->get_daemon_status_text().contains("Reload failed"));
        QVERIFY(editor->is_dirty());
        QCOMPARE(output(editor)->get_scale(), 2.0F);
        f.write(name, original);
        // A directory at the destination blocks writing even when the container runs as root.
        QVERIFY(QFile::remove(f.dir.filePath(name)));
        QVERIFY(QDir().mkpath(f.dir.filePath(name)));
        editor->save_profile();
        SETTLE(window);
        QVERIFY(editor->is_dirty());
        QCOMPARE(editor->get_selected_profile_id(), QString("desk"));
        QCOMPARE(output(editor)->get_scale(), 2.0F);
        QVERIFY(editor->get_save_status_text().contains("not saved"));
        QVERIFY(QDir(f.dir.filePath(name)).removeRecursively());
        f.write(name, original);
        // The recreated file has a new version; explicitly reload before reconciling edits.
        editor->reload_config_from_disk();
        output(editor)->set_scale(2);
        editor->save_profile();
        SETTLE(window);
        QVERIFY2(!editor->is_dirty(), qPrintable(editor->get_daemon_status_text()));
        editor->reload_config_from_disk();
        QCOMPARE(output(editor)->get_scale(), 2.0F);
    }

    void save_conflict_data() { lifecycle_data(); }

    void save_conflict() {
        QFETCH(int, mode);
        Fixture f;
        f.profiles();
        MainWindow window;
        SETTLE(window);
        window.set_backend_mode(mode);
        SETTLE(window);
        auto * editor = window.get_profile_editor();
        QVERIFY(editor->select_profile("desk"));
        output(editor)->set_scale(2);
        const auto * name = config_name(mode);
        const auto external = f.read(name) + "# changed by another editor\n";
        f.write(name, external);
        editor->save_profile();
        SETTLE(window);
        QCOMPARE(f.read(name), external);
        QVERIFY(editor->is_dirty());
        QCOMPARE(output(editor)->get_scale(), 2.0F);
        QVERIFY(editor->get_save_status_text().contains("not saved"));
        QVERIFY(editor->get_save_status_text().contains("reload"));
        QCOMPARE(f.read("daemon_calls").find("reload"), std::string::npos);
    }

    void saved_reload_failure_data() {
        QTest::addColumn<int>("mode");
        QTest::addColumn<bool>("remove");
        for (const int mode : { 1, 2 }) {
            QTest::newRow(qPrintable(QString("save-%1").arg(mode))) << mode << false;
            QTest::newRow(qPrintable(QString("delete-%1").arg(mode))) << mode << true;
        }
    }

    void saved_reload_failure() {
        QFETCH(int, mode);
        QFETCH(bool, remove);
        Fixture f;
        f.profiles();
        qputenv("WAYLAND_DISPLAY", "test-display");
        f.write("fr.emersion.kanshi.test-display", "");
        f.write("auto-wlr-randr/auto-wlr-randr.sock", "");
        MainWindow window;
        SETTLE(window);
        window.set_backend_mode(mode);
        SETTLE(window);
        auto * editor = window.get_profile_editor();
        QVERIFY(editor->select_profile("desk"));
        output(editor)->set_scale(2);
        const auto before = f.read(config_name(mode));
        f.write("daemon_mode", "reload_exit");
        if (remove) {
            editor->delete_profile("spare");
        } else {
            editor->save_profile();
        }
        SETTLE(window);
        QVERIFY(!editor->is_dirty());
        QVERIFY(f.read(config_name(mode)) != before);
        QVERIFY(editor->get_save_status_text().contains("saved to disk"));
        QVERIFY(editor->get_save_status_text().contains("reload failed"));
        QVERIFY(editor->get_save_status_text().contains("exit code 17"));
        QVERIFY(editor->get_save_status_text().contains("daemon failure detail"));
        editor->refresh_daemon_status();
        SETTLE(window);
        QVERIFY(editor->get_save_status_text().contains("reload failed"));
        // Discard uses the saved disk version even though the daemon rejected reload.
        output(editor)->set_scale(3);
        editor->discard_changes();
        QCOMPARE(output(editor)->get_scale(), 2.0F);
        if (remove) {
            QVERIFY(!editor->get_profile_ids().contains("spare"));
        }
        f.write("daemon_mode", "success");
        output(editor)->set_scale(2.5F);
        editor->save_profile();
        SETTLE(window);
        QVERIFY2(!editor->is_dirty(), qPrintable(editor->get_save_status_text()));
        QVERIFY(editor->get_save_status_text().contains("reload succeeded"));
        editor->reload_config_from_disk();
        QCOMPARE(output(editor)->get_scale(), 2.5F);
    }
};
QTEST_MAIN(ProfileLifecycleTests)
#include "profile_lifecycle.moc"
