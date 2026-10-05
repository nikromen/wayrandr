#include <qtestcase.h>
#include <qtmetamacros.h>

#include <QFile>
#include <QObject>
#include <QString>
#include <QTest>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "backend/auto_wlr_randr/config_repository.hpp"
#include "backend/auto_wlr_randr/conversions.hpp"
#include "backend/kanshi/config_parser.hpp"
#include "backend/kanshi/config_repository.hpp"
#include "backend/kanshi/conversions.hpp"
#include "models/main_window.hpp"
#include "models/profile/profile_output_properties.hpp"
#include "support/fixture.hpp"
#include "utils/helpers.hpp"

#define SETTLE(window) QTRY_VERIFY_WITH_TIMEOUT(!(window).is_busy(), 20000)

class ConfigRoundtripTests : public QObject {
    Q_OBJECT
private slots:

    void cleanupTestCase() { shutdown_commands(); }  // NOLINT: Qt Test hook name.

    void kanshi_document() {
        Fixture fixture;
        fixture.write("config", R"(# Main file only: do not load or execute the include.
include '/path/never-expanded/$(payload)'
output "Panel 🖥" { alias $panel
    disable
    adaptive_sync on
}
profile "output" {
    # comment between complete directives
    ...output "Panel 🖥" scale +1.25
    exec echo "hello world" # comment must not consume the closing brace
}
profile {
    output DP-2 mode --custom 1280x720@75Hz position -10,+20
}
)");
        KanshiConfigRepository repository(fixture.dir.filePath("config").toStdString());
        const auto before = repository.load();
        QCOMPARE(before.profiles.size(), size_t{ 2 });
        QVERIFY(before.profiles[0].outputs[0].multi_output);
        QVERIFY(before.profiles[1].anonymous);
        auto document = kanshi_conversions::to_profile_document(before);
        ProfileOutputProperties output(nullptr);
        output.load_from_output(document.profiles[0].outputs[0]);
        output.set_transform("90");
        document.profiles[0].outputs[0] = output.to_output();
        const auto native = kanshi_conversions::from_profile_document(document);
        (void)repository.save(native);
        const auto after = repository.load();
        QCOMPARE(after.includes, before.includes);
        QCOMPARE(after.global_outputs[0].criteria, before.global_outputs[0].criteria);
        QCOMPARE(after.global_outputs[0].alias, before.global_outputs[0].alias);
        QCOMPARE(after.global_outputs[0].enabled, before.global_outputs[0].enabled);
        QCOMPARE(after.global_outputs[0].adaptive_sync, before.global_outputs[0].adaptive_sync);
        QCOMPARE(after.profiles[0].id, std::string("output"));
        QVERIFY(after.profiles[0].outputs[0].multi_output);
        QVERIFY(!after.profiles[0].outputs[0].enabled.has_value());
        QVERIFY(!after.profiles[0].outputs[0].adaptive_sync.has_value());
        QCOMPARE(after.profiles[0].outputs[0].scale, before.profiles[0].outputs[0].scale);
        QCOMPARE(after.profiles[0].outputs[0].transform, std::optional<std::string>("90"));
        QCOMPARE(after.profiles[0].exec, before.profiles[0].exec);
        QVERIFY(after.profiles[1].anonymous);
        QCOMPARE(after.profiles[1].outputs[0].mode, before.profiles[1].outputs[0].mode);
        QCOMPARE(after.profiles[1].outputs[0].position, before.profiles[1].outputs[0].position);
        QCOMPARE(kanshi_config_parser::serialize(after), kanshi_config_parser::serialize(native));
    }

    void auto_document() {
        Fixture fixture;
        fixture.write("config.toml", R"(# Declaration order affects backend profile priority.
on_no_match_exec = ["""echo
second line 🖥"""]
[profile.z_first]
exec = ['echo "quotes"', 'echo \\path']
[[profile.z_first.settings]]
output = 'DP-1'
preferred = false
mode = '1920x1080@60Hz'
right_of = 'HDMI-1'
above = 'DP-3'
scale = 1
[[profile.z_first.settings]]
output = 'DP-2'
on = true
adaptive_sync = false
transform = 'normal'
[profile.a_second]
exec = []
)");
        AutoWlrRandrConfigRepository repository(fixture.dir.filePath("config.toml").toStdString());
        const auto before = repository.load();
        QCOMPARE(before.profiles[0].id, std::string("z_first"));
        auto document = auto_wlr_randr_conversions::to_profile_document(before);
        ProfileOutputProperties output(nullptr);
        output.load_from_output(document.profiles[0].outputs[0]);
        output.set_scale(1.5F);
        document.profiles[0].outputs[0] = output.to_output();
        (void)repository.save(auto_wlr_randr_conversions::from_profile_document(document));
        const auto after = repository.load();
        QCOMPARE(after.profiles[0].id, std::string("z_first"));
        QCOMPARE(after.profiles[1].id, std::string("a_second"));
        QCOMPARE(after.on_no_match_exec, before.on_no_match_exec);
        QCOMPARE(after.profiles[0].exec, before.profiles[0].exec);
        const auto & first = after.profiles[0].settings[0];
        QVERIFY(!first.on.has_value());
        QVERIFY(!first.adaptive_sync.has_value());
        QVERIFY(first.preferred_explicit);
        QVERIFY(!first.preferred);
        QCOMPARE(first.mode, before.profiles[0].settings[0].mode);
        QCOMPARE(first.right_of, before.profiles[0].settings[0].right_of);
        QCOMPARE(first.above, before.profiles[0].settings[0].above);
        QCOMPARE(first.scale, std::optional<float>(1.5F));
        QCOMPARE(after.profiles[0].settings[1].on, std::optional<bool>(true));
        QCOMPARE(after.profiles[0].settings[1].adaptive_sync, std::optional<bool>(false));
        QCOMPARE(after.profiles[0].settings[1].transform, std::optional<std::string>("normal"));
    }

    void auto_dotted_order() {
        Fixture fixture;
        fixture.write("config.toml", "profile.z.exec = ['first']\nprofile.a.exec = ['second']\n");
        AutoWlrRandrConfigRepository repository(fixture.dir.filePath("config.toml").toStdString());
        const auto document = repository.load();
        QCOMPARE(document.profiles[0].id, std::string("z"));
        QCOMPARE(document.profiles[1].id, std::string("a"));
        (void)repository.save(document);
        const auto after = repository.load();
        QCOMPARE(after.profiles[0].id, std::string("z"));
        QCOMPARE(after.profiles[1].id, std::string("a"));
    }

    void invalid_kanshi_data() {
        QTest::addColumn<std::string>("content");
        QTest::newRow("unknown-root") << std::string("future value\n");
        QTest::newRow("unknown-profile") << std::string("profile p {\n future value\n}\n");
        QTest::newRow("unknown-output")
            << std::string("profile p {\n output DP-1 future value\n}\n");
        QTest::newRow("missing-include") << std::string("include\nprofile p {}\n");
        QTest::newRow("extra-include") << std::string("include one two\n");
        QTest::newRow("include-order") << std::string("profile p {}\ninclude /dev/null\n");
        QTest::newRow("include-default-order")
            << std::string("output DP-1 alias $panel\ninclude /dev/null\n");
        QTest::newRow("duplicate-profile") << std::string("profile p {}\nprofile p {}\n");
        QTest::newRow("duplicate-output")
            << std::string("profile p {\noutput DP-1\noutput DP-1\n}\n");
        QTest::newRow("duplicate-global") << std::string("output DP-1\noutput DP-1\n");
        QTest::newRow("conflicting-mode")
            << std::string("output DP-1 mode preferred mode 1920x1080\n");
        QTest::newRow("missing-output") << std::string("profile p {\noutput\n}\n");
        QTest::newRow("missing-mode") << std::string("profile p {\noutput DP-1 mode\n}\n");
        QTest::newRow("incomplete-block") << std::string("profile p {\noutput DP-1 {\nscale 1\n");
        QTest::newRow("quoted-newline") << std::string("profile \"bad\nname\" {}\n");
        QTest::newRow("quote") << std::string("profile \"bad");
        QTest::newRow("stray-brace") << std::string("}\n");
        QTest::newRow("bad-transform") << std::string("output DP-1 transform upside-down\n");
        QTest::newRow("double-sign") << std::string("output DP-1 scale +-1\n");
        QTest::newRow("position-double-sign") << std::string("output DP-1 position +-1,0\n");
        QTest::newRow("profile-alias") << std::string("profile p {\noutput DP-1 alias $p\n}\n");
        QTest::newRow("nul") << std::string("profile p {}\0trailing", 22);
    }

    void invalid_kanshi() {
        QFETCH(std::string, content);
        Fixture fixture;
        fixture.write("config", content);
        KanshiConfigRepository repository(fixture.dir.filePath("config").toStdString());
        QVERIFY_THROWS_EXCEPTION(kanshi_config_parser::ParseError, (void)repository.load());
        QCOMPARE(fixture.read("config"), content);
    }

    void invalid_auto_data() {
        QTest::addColumn<std::string>("content");
        QTest::newRow("unknown-root") << std::string("future = 1\n[profile.p]\n");
        QTest::newRow("unknown-profile") << std::string("[profile.p]\nfuture = 1\n");
        QTest::newRow("unknown-output")
            << std::string("[profile.p]\nsettings = [{ output = 'DP-1', future = 1 }]\n");
        QTest::newRow("missing-profile") << std::string("# only a comment\n");
        QTest::newRow("profile-type") << std::string("profile = []\n");
        QTest::newRow("profile-entry-type") << std::string("[profile]\np = 3\n");
        QTest::newRow("exec-type") << std::string("[profile.p]\nexec = 'echo'\n");
        QTest::newRow("exec-member-type") << std::string("[profile.p]\nexec = ['echo', 1]\n");
        QTest::newRow("unmatched-type") << std::string("on_no_match_exec = [false]\n[profile.p]\n");
        QTest::newRow("settings-type") << std::string("[profile.p]\nsettings = {}\n");
        QTest::newRow("settings-member-type") << std::string("[profile.p]\nsettings = [42]\n");
        QTest::newRow("missing-output") << std::string("[profile.p]\nsettings = [{ on = true }]\n");
        QTest::newRow("output-type") << std::string("[profile.p]\nsettings = [{ output = 42 }]\n");
        QTest::newRow("bool-type")
            << std::string("[profile.p]\nsettings = [{ output = 'DP-1', preferred = 'false' }]\n");
        QTest::newRow("scale-type")
            << std::string("[profile.p]\nsettings = [{ output = 'DP-1', scale = '1.5' }]\n");
        QTest::newRow("duplicate-profile") << std::string("[profile.p]\n[profile.p]\n");
        QTest::newRow("duplicate-key") << std::string("[profile.p]\nexec = []\nexec = ['echo']\n");
        QTest::newRow("syntax") << std::string("[profile.p\n");
    }

    void invalid_auto() {
        QFETCH(std::string, content);
        Fixture fixture;
        fixture.write("config.toml", content);
        AutoWlrRandrConfigRepository repository(fixture.dir.filePath("config.toml").toStdString());
        QVERIFY_THROWS_EXCEPTION(std::exception, (void)repository.load());
        QCOMPARE(fixture.read("config.toml"), content);
    }

    void load_error_data() {
        QTest::addColumn<int>("mode");
        QTest::newRow("kanshi") << 1;
        QTest::newRow("auto-wlr-randr") << 2;
    }

    void load_error() {
        QFETCH(int, mode);
        Fixture fixture;
        fixture.profiles();
        std::string name = "kanshi/config";
        if (mode == 2) {
            name = "auto-wlr-randr/config.toml";
        }
        const auto valid = fixture.read(name.c_str());
        const std::string broken = "unsupported = 42\n";
        fixture.write(name.c_str(), broken);
        MainWindow window;
        SETTLE(window);
        window.set_backend_mode(mode);
        SETTLE(window);
        auto * editor = window.get_profile_editor();
        QVERIFY(!editor->is_configuration_loaded());
        QVERIFY(!editor->get_load_error().isEmpty());
        editor->create_profile_from_live("new");
        SETTLE(window);
        QVERIFY(editor->is_dirty());
        editor->save_profile();
        SETTLE(window);
        QCOMPARE(fixture.read(name.c_str()), broken);
        QVERIFY(editor->is_dirty());
        QVERIFY(editor->get_save_status_text().contains("not saved"));
        fixture.write(name.c_str(), valid);
        editor->reload_config_from_disk();
        QVERIFY(editor->is_configuration_loaded());
        QVERIFY(editor->get_load_error().isEmpty());
        QCOMPARE(editor->get_selected_profile_id(), QStringLiteral("desk"));
        QVERIFY(editor->select_profile("desk"));
        editor->add_exec_command("echo preserved");
        fixture.write(name.c_str(), broken);
        editor->reload_config_from_disk();
        QVERIFY(editor->is_dirty());
        QVERIFY(editor->get_exec_commands().contains("echo preserved"));
        editor->save_profile();
        SETTLE(window);
        QCOMPARE(fixture.read(name.c_str()), broken);
        QVERIFY(editor->is_dirty());
    }

    void large_and_incomplete() {
        std::string content;
        for (int index = 0; index < 1000; ++index) {
            content += "profile p" + std::to_string(index) + " {\noutput DP-1 scale 1\n}\n";
        }
        QCOMPARE(kanshi_config_parser::parse(content).profiles.size(), size_t{ 1000 });
        content += "profile unfinished {\noutput DP-1 {\nscale";
        QVERIFY_THROWS_EXCEPTION(
            kanshi_config_parser::ParseError, (void)kanshi_config_parser::parse(content)
        );
        const std::string nested = "profile p {\n" + std::string(2048, '{');
        QVERIFY_THROWS_EXCEPTION(
            kanshi_config_parser::ParseError, (void)kanshi_config_parser::parse(nested)
        );
    }
};

QTEST_MAIN(ConfigRoundtripTests)
#include "config_roundtrip.moc"
