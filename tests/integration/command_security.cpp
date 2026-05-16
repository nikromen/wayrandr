#include <qcontainerfwd.h>
#include <qtenvironmentvariables.h>
#include <qtestcase.h>
#include <qtmetamacros.h>

#include <QTest>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "backend/auto_wlr_randr/config_repository.hpp"
#include "backend/auto_wlr_randr/daemon_client.hpp"
#include "backend/auto_wlr_randr/types.hpp"
#include "backend/kanshi/config_parser.hpp"
#include "backend/kanshi/config_repository.hpp"
#include "backend/kanshi/daemon_client.hpp"
#include "backend/kanshi/types.hpp"
#include "backend/wlr_randr.hpp"
#include "models/main_window.hpp"
#include "monitor_specs.hpp"
#include "support/environment.hpp"
#include "support/fixture.hpp"
#include "utils/helpers.hpp"

class CommandSecurityTests : public QObject {
    Q_OBJECT
private slots:

    void cleanupTestCase() { shutdown_commands(); }  // NOLINT: Qt Test hook name.

    void values_data() {
        QTest::addColumn<std::string>("value");
        QTest::newRow("dash") << std::string("--force");
        QTest::newRow("output-option") << std::string("--off");
        QTest::newRow("output-selector") << std::string("--output");
        QTest::newRow("opening-brace") << std::string("{");
        QTest::newRow("closing-brace") << std::string("}");
        QTest::newRow("empty") << std::string();
        QTest::newRow("quotes") << std::string("Panel \"quoted\" 'single'");
        QTest::newRow("backslash") << std::string("Panel\\path\\");
        QTest::newRow("unicode") << std::string("Příliš žluťoučký 🖥");
        QTest::newRow("syntax") << std::string("} profile injected { exec payload # $(payload);");
        QTest::newRow("controls") << std::string("tab\tcarriage\rcontrol\x01");
    }

    void values() {
        QFETCH(std::string, value);
        Fixture fixture;
        KanshiOutputSetting output;
        output.criteria = value;
        output.enabled = true;
        KanshiProfile profile;
        profile.id = value;
        profile.outputs.push_back(output);
        profile.exec = { R"(echo "quoted" 'single' \{ \} $HOME; echo $(payload))", "echo x\\ " };
        KanshiConfig config;
        config.profiles.push_back(profile);
        const auto serialized = kanshi_config_parser::serialize(config);
        const auto parsed = kanshi_config_parser::parse(serialized);
        QCOMPARE(parsed.profiles.size(), size_t{ 1 });
        QCOMPARE(parsed.profiles[0].id, value);
        QCOMPARE(parsed.profiles[0].outputs.size(), size_t{ 1 });
        QCOMPARE(parsed.profiles[0].outputs[0].criteria, value);
        QCOMPARE(parsed.profiles[0].exec, profile.exec);
        QVERIFY(parsed.preserved_directives.empty());

        AutoWlrRandrProfile auto_profile;
        auto_profile.id = value;
        ProfileOutputSetting auto_output;
        auto_output.output = value;
        auto_output.right_of = value;
        auto_profile.settings.push_back(auto_output);
        auto_profile.exec = { value };
        AutoWlrRandrConfig auto_config;
        auto_config.profiles.push_back(auto_profile);
        auto_config.on_no_match_exec = { value };
        AutoWlrRandrConfigRepository repository(fixture.dir.filePath("config.toml").toStdString());
        static_cast<void>(repository.save(auto_config));
        const auto reloaded = repository.load();
        QCOMPARE(reloaded.profiles.size(), size_t{ 1 });
        QCOMPARE(reloaded.profiles[0].id, value);
        QCOMPARE(reloaded.profiles[0].settings[0].output, value);
        QCOMPARE(reloaded.profiles[0].settings[0].right_of.value(), value);
        QCOMPARE(reloaded.profiles[0].exec, auto_profile.exec);
        QCOMPARE(reloaded.on_no_match_exec, auto_config.on_no_match_exec);

        fixture.install(FAKE_COMMAND, "argv");
        QVERIFY(is_program_available("argv"));
        auto result = start_command(this, "argv", { "argv", value, "", "second argument" });
        QTRY_VERIFY_WITH_TIMEOUT(result->done, 10000);
        QVERIFY2(result->error.empty(), result->error.c_str());
        QCOMPARE(json::parse(result->output), json({ value, "", "second argument" }));

        auto monitors = fixture.state();
        monitors[0]["name"] = value;
        fixture.write("state", monitors.dump());
        auto built = std::make_shared<JobResult>();
        run_job(
            this,
            [built] {
                WlrRandrBackend backend;
                backend.apply(get_monitor_specs_list());
                backend.confirm();
                return [built] { built->done = true; };
            },
            [built](const std::string & error) {
                built->error = error;
                built->done = true;
            }
        );
        QTRY_VERIFY_WITH_TIMEOUT(built->done, 10000);
        QVERIFY2(built->error.empty(), built->error.c_str());
        std::istringstream calls(fixture.read("calls"));
        std::string line;
        json arguments;
        while (std::getline(calls, line)) {
            arguments = json::parse(line);
        }
        QCOMPARE(arguments[0], json("--output"));
        QCOMPARE(arguments[1], json(value));
        QCOMPARE(arguments[2], json("--on"));
    }

    void invalid_kanshi_exec_data() {
        QTest::addColumn<std::string>("command");
        QTest::newRow("newline") << std::string("echo safe\n}\nprofile injected {\nexec payload");
        QTest::newRow("nul") << std::string("echo\0payload", 12);
        QTest::newRow("block") << std::string("echo safe { exec payload }");
        QTest::newRow("empty-block") << std::string("echo safe { }");
        QTest::newRow("closing-brace") << std::string("echo safe } profile injected {");
        QTest::newRow("escape") << std::string("echo safe\\");
        QTest::newRow("quote") << std::string("echo \"unterminated");
    }

    void valid_kanshi_exec_data() {
        QTest::addColumn<std::string>("command");
        QTest::newRow("quoted-braces") << std::string("echo '{ }' \"quoted\"");
        QTest::newRow("escaped-braces") << std::string("echo \\{ \\}");
        QTest::newRow("shell-operators")
            << std::string("echo $(payload); echo $HOME | cat && true");
        QTest::newRow("escaped-trailing-space") << std::string("echo x\\ ");
        QTest::newRow("single-quoted-backslash") << std::string("echo '\\'");
        QTest::newRow("adjacent-quoted-words") << std::string("echo 'one'\"two\"");
    }

    void valid_kanshi_exec() {
        QFETCH(std::string, command);
        KanshiConfig config;
        KanshiProfile profile;
        profile.id = "safe";
        profile.exec = { command };
        config.profiles.push_back(profile);
        const auto content = kanshi_config_parser::serialize(config);
        QCOMPARE(kanshi_config_parser::parse(content).profiles[0].exec, profile.exec);
        QVERIFY(content.find("wayrandr_exec_boundary") == std::string::npos);
    }

    void invalid_kanshi_exec() {
        QFETCH(std::string, command);
        Fixture fixture;
        fixture.write("config", "original content");
        KanshiConfig config;
        KanshiProfile profile;
        profile.id = "safe";
        profile.exec = { command };
        config.profiles.push_back(profile);
        KanshiConfigRepository repository(fixture.dir.filePath("config").toStdString());
        QVERIFY_THROWS_EXCEPTION(
            kanshi_config_parser::ParseError, static_cast<void>(repository.save(config))
        );
        QCOMPARE(fixture.read("config"), std::string("original content"));
    }

    void unsupported_kanshi_values() {
        for (const auto & value : { std::string("name\ninjected"), std::string("name\0tail", 9) }) {
            KanshiConfig config;
            KanshiProfile profile;
            profile.id = value;
            config.profiles.push_back(profile);
            QVERIFY_THROWS_EXCEPTION(
                kanshi_config_parser::ParseError, (void)kanshi_config_parser::serialize(config)
            );
            QVERIFY_THROWS_EXCEPTION(
                kanshi_config_parser::ParseError,
                (void)kanshi_config_parser::parse("profile \"" + value + "\" {\n}\n")
            );
        }
        const auto bare = kanshi_config_parser::parse("profile dash\\ name {\noutput 'DP\\1'\n}\n");
        QCOMPARE(bare.profiles[0].id, std::string("dash name"));
        QCOMPARE(bare.profiles[0].outputs[0].criteria, std::string("DP\\1"));
    }

    void toml_multiline() {
        Fixture fixture;
        const std::string value = "\"\\\n[profile.injected]\nexec = ['payload']\n🖥";
        AutoWlrRandrConfig config;
        AutoWlrRandrProfile profile;
        profile.id = value;
        profile.exec = { value };
        ProfileOutputSetting output;
        output.output = value;
        profile.settings.push_back(output);
        config.profiles.push_back(profile);
        config.on_no_match_exec = { value };
        AutoWlrRandrConfigRepository repository(fixture.dir.filePath("config.toml").toStdString());
        static_cast<void>(repository.save(config));
        const auto loaded = repository.load();
        QCOMPARE(loaded.profiles.size(), size_t{ 1 });
        QCOMPARE(loaded.profiles[0].id, value);
        QCOMPARE(loaded.profiles[0].settings[0].output, value);
        QCOMPARE(loaded.profiles[0].exec, profile.exec);
        QCOMPARE(loaded.on_no_match_exec, config.on_no_match_exec);
    }

    void daemon_arguments() {
        Fixture fixture;
        fixture.profiles();
        qputenv("WAYLAND_DISPLAY", "test-display");
        fixture.write("fr.emersion.kanshi.test-display", "");
        fixture.write("auto-wlr-randr/auto-wlr-randr.sock", "");
        const std::string profile = "--force \"Příliš\"\\\n$(payload)";
        auto state = std::make_shared<JobResult>();
        run_job(
            this,
            [state, profile] {
                KanshiDaemonClient::switch_profile(profile);
                AutoWlrRandrDaemonClient::switch_profile(profile, false);
                AutoWlrRandrDaemonClient::switch_profile(profile, true);
                return [state] { state->done = true; };
            },
            [state](const std::string & error) {
                state->error = error;
                state->done = true;
            }
        );
        QTRY_VERIFY_WITH_TIMEOUT(state->done, 10000);
        QVERIFY2(state->error.empty(), state->error.c_str());
        std::istringstream log(fixture.read("daemon_calls"));
        std::string line;
        QVERIFY(static_cast<bool>(std::getline(log, line)));
        QCOMPARE(json::parse(line), json({ "kanshictl", "switch", profile }));
        QVERIFY(static_cast<bool>(std::getline(log, line)));
        QCOMPARE(json::parse(line), json({ "auto-wlr-randrctl", "switch", "--", profile }));
        QVERIFY(static_cast<bool>(std::getline(log, line)));
        QCOMPARE(
            json::parse(line), json({ "auto-wlr-randrctl", "switch", "--force", "--", profile })
        );
        QVERIFY(!std::getline(log, line));
    }

    void invalid_process_strings() {
        Environment environment;
        for (const auto & value : { std::string("a\0b", 3), std::string("\xff", 1) }) {
            auto result = start_command(this, "/usr/bin/python3", { FAKE_COMMAND, "args", value });
            QTRY_VERIFY_WITH_TIMEOUT(result->done, 10000);
            QVERIFY(result->error.find("UTF-8 without NUL") != std::string::npos);
            QVERIFY_THROWS_EXCEPTION(std::invalid_argument, (void)is_program_available(value));
        }
    }

    void loaded_exec_is_only_edited() {
        Fixture fixture;
        fixture.profiles();
        MainWindow window;
        QTRY_VERIFY_WITH_TIMEOUT(!window.is_busy(), 20000);
        window.set_backend_mode(2);
        QTRY_VERIFY_WITH_TIMEOUT(!window.is_busy(), 20000);
        auto * editor = window.get_profile_editor();
        QVERIFY(editor->select_profile("desk"));
        QCOMPARE(editor->get_exec_commands(), QStringList({ "notify-send desk" }));
        const auto before = fixture.read("daemon_calls");
        editor->set_exec_command(0, "$(payload); echo 'command'\\\nsecond line");
        editor->add_exec_command("another command");
        editor->remove_exec_command(1);
        editor->set_on_no_match_exec_command(0, "$(other_payload)");
        drain_jobs();
        QCOMPARE(fixture.read("daemon_calls"), before);
        QVERIFY(editor->is_dirty());
    }

    void invalid_runtime_environment() {
        Environment environment;
        qputenv("WAYLAND_DISPLAY", "test");
        for (const auto * runtime : { "", "relative/path" }) {
            qputenv("XDG_RUNTIME_DIR", runtime);
            QVERIFY_THROWS_EXCEPTION(std::runtime_error, (void)KanshiDaemonClient::is_running());
            QVERIFY_THROWS_EXCEPTION(
                std::runtime_error, (void)AutoWlrRandrDaemonClient::is_running()
            );
        }
        qputenv("XDG_RUNTIME_DIR", environment.dir.path().toUtf8());
        qputenv("WAYLAND_DISPLAY", "");
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, (void)KanshiDaemonClient::is_running());
        qputenv("XDG_CONFIG_HOME", "relative/config");
        QVERIFY(KanshiConfigRepository::default_config_path().is_absolute());
        QVERIFY(AutoWlrRandrConfigRepository::default_config_path().is_absolute());
    }
};

QTEST_GUILESS_MAIN(CommandSecurityTests)
#include "command_security.moc"
