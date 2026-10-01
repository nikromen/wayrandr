#include <qtestcase.h>
#include <qtmetamacros.h>

#include <QFile>
#include <QObject>
#include <QPoint>
#include <QSignalSpy>
#include <QString>
#include <QTest>
#include <QVariant>
#include <cmath>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "backend/auto_wlr_randr/config_repository.hpp"
#include "backend/auto_wlr_randr/conversions.hpp"
#include "backend/base.hpp"
#include "backend/kanshi/config_parser.hpp"
#include "backend/kanshi/config_repository.hpp"
#include "backend/kanshi/conversions.hpp"
#include "backend/profile/types.hpp"
#include "models/main_window.hpp"
#include "models/profile/profile_editor_controller.hpp"
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

    void auto_transform_data() {
        QTest::addColumn<std::string>("transform");
        QTest::addColumn<bool>("specified");
        QTest::addColumn<bool>("valid");
        QTest::addColumn<int>("width");
        QTest::addColumn<int>("height");
        QTest::newRow("unset") << std::string() << false << true << 1280 << 720;
        QTest::newRow("normal") << std::string("normal") << true << true << 1280 << 720;
        QTest::newRow("90") << std::string("90") << true << true << 720 << 1280;
        QTest::newRow("180") << std::string("180") << true << true << 1280 << 720;
        QTest::newRow("270") << std::string("270") << true << true << 720 << 1280;
        QTest::newRow("flipped") << std::string("flipped") << true << true << 1280 << 720;
        QTest::newRow("flipped-90") << std::string("flipped-90") << true << true << 720 << 1280;
        QTest::newRow("flipped-180") << std::string("flipped-180") << true << true << 1280 << 720;
        QTest::newRow("flipped-270") << std::string("flipped-270") << true << true << 720 << 1280;
        QTest::newRow("empty") << std::string() << true << false << 1280 << 720;
        QTest::newRow("unknown") << std::string("invalid") << true << false << 1280 << 720;
    }

    void auto_transform() {
        QFETCH(std::string, transform);
        QFETCH(bool, specified);
        QFETCH(bool, valid);
        Fixture fixture;
        std::string content = "[profile.desk]\nsettings = [{ output = 'DP-1'";
        std::optional<std::string> expected;
        if (specified) {
            expected = transform;
            content += ", transform = '" + transform + "'";
        }
        content += " }]\n";
        fixture.write("config.toml", content);
        AutoWlrRandrConfigRepository repository(fixture.dir.filePath("config.toml").toStdString());
        if (valid) {
            const auto config = repository.load();
            QCOMPARE(config.profiles[0].settings[0].transform, expected);
            auto document = auto_wlr_randr_conversions::to_profile_document(config);
            ProfileOutputProperties output(nullptr);
            output.load_from_output(document.profiles[0].outputs[0]);
            output.set_mode("1280x720");
            document.profiles[0].outputs[0] = output.to_output();
            (void)repository.save(auto_wlr_randr_conversions::from_profile_document(document));
            const auto after = repository.load();
            QCOMPARE(after.profiles[0].settings[0].transform, expected);
            QVERIFY(!after.profiles[0].settings[0].scale.has_value());
        } else {
            std::string error;
            try {
                (void)repository.load();
            } catch (const std::runtime_error & e) {
                error = e.what();
            }
            QVERIFY(error.find("Unsupported transform '" + transform + "'") != std::string::npos);
            QVERIFY(error.find("profile 'desk'") != std::string::npos);
            QVERIFY(error.find("output 'DP-1'") != std::string::npos);
            QCOMPARE(fixture.read("config.toml"), content);
        }
    }

    void model_transform_data() { auto_transform_data(); }

    void model_transform() {
        QFETCH(std::string, transform);
        QFETCH(bool, specified);
        QFETCH(int, width);
        QFETCH(int, height);
        profile::ProfileOutputDefinition definition;
        definition.output = "DP-1";
        definition.mode = "2560x1440@60Hz";
        definition.scale = 2.0F;
        if (specified) {
            definition.transform = transform;
        }
        ProfileOutputProperties output(nullptr);
        output.load_from_output(definition);
        QCOMPARE(output.get_layout_width(), width);
        QCOMPARE(output.get_layout_height(), height);
        // Exercise the Qt property boundary used by QML, including repeated reads.
        QCOMPARE(output.property("layoutWidth").toInt(), width);
        QCOMPARE(output.property("layoutHeight").toInt(), height);
        QCOMPARE(output.to_output().transform, definition.transform);
        output.set_adaptive_sync(true);
        QCOMPARE(output.to_output().transform, definition.transform);
    }

    void kanshi_custom_geometry_data() {
        QTest::addColumn<QString>("options");
        QTest::addColumn<int>("width");
        QTest::addColumn<int>("height");
        QTest::newRow("native-size") << QString("scale 1 transform normal") << 2560 << 1440;
        QTest::newRow("scaled") << QString("scale 2 transform normal") << 1280 << 720;
        QTest::newRow("scaled-flipped-rotation")
            << QString("scale 2 transform flipped-90") << 720 << 1280;
    }

    void kanshi_custom_geometry() {
        QFETCH(QString, options);
        QFETCH(int, width);
        QFETCH(int, height);
        Fixture fixture;
        fixture.profiles();
        const auto content =
            QString(
                "profile desk {\n"
                "output DP-1 enable mode --custom 2560x1440@60Hz %1\n"
                "output DP-2 enable mode 1920x1080@60Hz scale 1 transform normal\n}\n"
            )
                .arg(options)
                .toStdString();
        fixture.write("kanshi/config", content);
        MainWindow window;
        SETTLE(window);
        window.set_backend_mode(editor_mode::KANSHI);
        SETTLE(window);
        auto * editor = window.get_profile_editor();
        QVERIFY(editor->is_configuration_loaded());
        QCOMPARE(editor->get_outputs().size(), 2);
        auto * first = qobject_cast<ProfileOutputProperties *>(editor->get_outputs()[0]);
        auto * second = qobject_cast<ProfileOutputProperties *>(editor->get_outputs()[1]);
        QVERIFY(first != nullptr && second != nullptr);
        const auto before = first->to_output();
        QCOMPARE(first->property("layoutWidth").toInt(), width);
        QCOMPARE(first->property("layoutHeight").toInt(), height);
        QCOMPARE(first->get_mode(), QString("--custom 2560x1440@60Hz"));
        QVERIFY(!editor->is_dirty());
        window.reset_canvas_layout(0, 1);
        QCOMPARE(first->get_position_x(), 0);
        QCOMPARE(second->get_position_x(), width);
        QCOMPARE(first->get_position_y(), 0);
        QCOMPARE(second->get_position_y(), 0);
        QCOMPARE(window.snap_position(second, width + 20, 0, 50, 0, 0, 1), QPoint(width, 0));
        QCOMPARE(first->to_output().mode, before.mode);
        QCOMPARE(first->to_output().scale, before.scale);
        QCOMPARE(first->to_output().transform, before.transform);
        QVERIFY(editor->is_dirty());
        QCOMPARE(fixture.read("kanshi/config"), content);
    }

    void approximate_geometry_data() {
        QTest::addColumn<int>("mode");
        QTest::addColumn<std::string>("content");
        QTest::addColumn<int>("native_width");
        const std::string second =
            "output DP-2 enable mode 1920x1080 scale 1 transform normal "
            "position 4000,50\n}\n";
        QTest::newRow("kanshi-unset")
            << static_cast<int>(editor_mode::KANSHI)
            << "profile desk {\noutput DP-1 enable scale 1 transform normal\n" + second << 3840;
        QTest::newRow("kanshi-preferred")
            << static_cast<int>(editor_mode::KANSHI)
            << "profile desk {\noutput DP-1 enable mode preferred scale 1 transform normal\n" +
                second
            << 3840;
        QTest::newRow("kanshi-global-scale")
            << static_cast<int>(editor_mode::KANSHI)
            << "output DP-1 scale 1.5\nprofile desk {\n"
               "output DP-1 enable mode 3840x2160 transform normal\n" +
                second
            << 2560;
        // auto-wlr-randr sends --preferred after --mode: the 4K preferred mode wins.
        QTest::newRow("auto-preferred-over-mode")
            << static_cast<int>(editor_mode::AUTO_WLR_RANDR)
            << std::string(
                   "[profile.desk]\nsettings = ["
                   "{ output = 'DP-1', on = true, mode = '1920x1080', preferred = true, "
                   "scale = 1, transform = 'normal' },"
                   "{ output = 'DP-2', on = true, mode = '1920x1080', scale = 1, "
                   "transform = 'normal', pos = '4000,50' }]\n"
               )
            << 3840;
    }

    void approximate_geometry() {
        QFETCH(int, mode);
        QFETCH(std::string, content);
        QFETCH(int, native_width);
        Fixture fixture;
        fixture.profiles();
        fixture.multi_monitor();
        auto state = fixture.state();
        state[0]["modes"][0]["width"] = 3840;
        state[0]["modes"][0]["height"] = 2160;
        fixture.write("state", state.dump());
        const char * path = "kanshi/config";
        if (mode == editor_mode::AUTO_WLR_RANDR) {
            path = "auto-wlr-randr/config.toml";
        }
        fixture.write(path, content);
        MainWindow window;
        SETTLE(window);
        window.set_backend_mode(mode);
        SETTLE(window);
        auto * editor = window.get_profile_editor();
        QCOMPARE(window.get_monitors().first()->property("resolutionWidth").toInt(), 3840);
        auto * first = qobject_cast<ProfileOutputProperties *>(editor->get_outputs().value(0));
        auto * second = qobject_cast<ProfileOutputProperties *>(editor->get_outputs().value(1));
        QVERIFY(first != nullptr && second != nullptr);
        const auto before = first->to_output();
        // Source-derived native expectations differ from the local-only preview.
        QVERIFY(first->get_layout_width() != native_width);
        QVERIFY(editor->has_approximate_layout());
        QVERIFY(!editor->is_dirty());
        window.reset_canvas_layout(0, 1);
        QCOMPARE(second->get_position_x(), 4000);
        QCOMPARE(second->get_position_y(), 50);
        QCOMPARE(window.snap_position(second, 1940, 50, 100, 0, 0, 1), QPoint(1940, 50));
        QVERIFY(!editor->is_dirty());
        QCOMPARE(first->to_output().mode, before.mode);
        QCOMPARE(first->to_output().scale, before.scale);
        QCOMPARE(first->to_output().preferred, before.preferred);
        QCOMPARE(first->to_output().transform, before.transform);
        QCOMPARE(fixture.read(path), content);
        QCOMPARE(fixture.state(), state);
    }

    void kanshi_nonfinite_geometry_data() {
        QTest::addColumn<std::string>("token");
        QTest::addColumn<float>("scale");
        QTest::newRow("nan") << std::string("nan") << std::numeric_limits<float>::quiet_NaN();
        QTest::newRow("infinity") << std::string("inf") << std::numeric_limits<float>::infinity();
        QTest::newRow("negative-infinity")
            << std::string("-inf") << -std::numeric_limits<float>::infinity();
        QTest::newRow("zero") << std::string("0") << 0.0F;
        QTest::newRow("negative") << std::string("-1") << -1.0F;
        QTest::newRow("tiny") << std::string("1e-30") << 1e-30F;
    }

    void kanshi_nonfinite_geometry() {
        QFETCH(std::string, token);
        QFETCH(float, scale);
        Fixture fixture;
        const auto content = "profile p {\noutput DP-1 scale " + token + "\n}\n";
        fixture.write("config", content);
        KanshiConfigRepository repository(fixture.dir.filePath("config").toStdString());
        const auto native = repository.load();
        const auto document = kanshi_conversions::to_profile_document(native);
        ProfileOutputProperties output(nullptr);
        output.load_from_output(document.profiles[0].outputs[0]);
        QSignalSpy changed(&output, &ProfileOutputProperties::setting_changed);
        QCOMPARE(output.property("layoutWidth").toInt(), 0);
        QCOMPARE(output.property("layoutHeight").toInt(), 0);
        const auto after = output.to_output();
        const auto roundtrip = kanshi_conversions::from_profile_document(document);
        for (const auto stored : { native.profiles[0].outputs[0].scale,
                                   document.profiles[0].outputs[0].scale,
                                   after.scale,
                                   roundtrip.profiles[0].outputs[0].scale }) {
            QVERIFY(stored.has_value());
            if (std::isnan(scale)) {
                QVERIFY(std::isnan(stored.value()));
            } else {
                QCOMPARE(stored.value(), scale);
            }
        }
        QCOMPARE(changed.count(), 0);
        QVERIFY(!after.mode && !after.transform && !after.pos);
        QCOMPARE(fixture.read("config"), content);
        QCOMPARE(fixture.read("calls"), std::string());
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
        QTest::addColumn<std::string>("broken");
        QTest::newRow("kanshi") << static_cast<int>(editor_mode::KANSHI)
                                << std::string("unsupported = 42\n");
        QTest::newRow("auto-wlr-randr")
            << static_cast<int>(editor_mode::AUTO_WLR_RANDR) << std::string("unsupported = 42\n");
        QTest::newRow("auto-invalid-transform")
            << static_cast<int>(editor_mode::AUTO_WLR_RANDR)
            << std::string(
                   "[profile.desk]\n"
                   "settings = [{ output = 'DP-1', transform = 'invalid' }]\n"
               );
    }

    void load_error() {
        QFETCH(int, mode);
        QFETCH(std::string, broken);
        Fixture fixture;
        fixture.profiles();
        std::string name = "kanshi/config";
        if (mode == editor_mode::AUTO_WLR_RANDR) {
            name = "auto-wlr-randr/config.toml";
        }
        const auto valid = fixture.read(name.c_str());
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
