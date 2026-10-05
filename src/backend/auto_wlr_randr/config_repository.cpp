#include "backend/auto_wlr_randr/config_repository.hpp"

#include <qhashfunctions.h>
#include <spdlog/spdlog.h>

#include <QStandardPaths>
#include <algorithm>
#include <filesystem>
#include <initializer_list>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
// The public entry point initializes dependencies required by toml++ internals.
#include <toml++/toml.h>  // IWYU pragma: keep

#include <utility>

#include "backend/auto_wlr_randr/types.hpp"
#include "utils/config_file.hpp"

namespace {

void check_keys(
    const toml::table & table,
    std::initializer_list<std::string_view> allowed,
    const std::string & location
) {
    for (const auto & [key, node] : table) {
        if (std::find(allowed.begin(), allowed.end(), key.str()) == allowed.end()) {
            throw std::runtime_error(
                "Unsupported TOML item at " + location + "." + std::string(key.str()) +
                "; edit this configuration manually to avoid losing data"
            );
        }
    }
}

template <typename Value>
auto optional_value(const toml::table & table, const char * key) -> std::optional<Value> {
    if (!table.contains(key)) {
        return std::nullopt;
    }
    const auto value = table[key].value<Value>();
    if (!value.has_value()) {
        throw std::runtime_error("Invalid TOML type for '" + std::string(key) + "'");
    }
    return value;
}

auto string_array(const toml::table & table, const char * key) -> std::vector<std::string> {
    std::vector<std::string> values;
    if (!table.contains(key)) {
        return values;
    }
    const auto * array = table[key].as_array();
    if (array == nullptr) {
        throw std::runtime_error("Expected TOML array for '" + std::string(key) + "'");
    }
    for (const auto & node : *array) {
        const auto value = node.value<std::string>();
        if (!value.has_value()) {
            throw std::runtime_error("Expected string in TOML array '" + std::string(key) + "'");
        }
        values.push_back(*value);
    }
    return values;
}

auto parse_output_setting(const toml::table & table) -> ProfileOutputSetting {
    check_keys(
        table,
        { "output",
          "on",
          "preferred",
          "adaptive_sync",
          "mode",
          "pos",
          "left_of",
          "right_of",
          "above",
          "below",
          "transform",
          "scale" },
        "settings"
    );
    ProfileOutputSetting setting;
    const auto output = optional_value<std::string>(table, "output");
    if (!output.has_value()) {
        throw std::runtime_error("TOML output setting requires a string 'output'");
    }
    setting.output = *output;
    setting.on = optional_value<bool>(table, "on");
    setting.preferred_explicit = table.contains("preferred");
    setting.preferred = optional_value<bool>(table, "preferred").value_or(false);
    setting.adaptive_sync = optional_value<bool>(table, "adaptive_sync");
    setting.mode = optional_value<std::string>(table, "mode");
    setting.pos = optional_value<std::string>(table, "pos");
    setting.left_of = optional_value<std::string>(table, "left_of");
    setting.right_of = optional_value<std::string>(table, "right_of");
    setting.above = optional_value<std::string>(table, "above");
    setting.below = optional_value<std::string>(table, "below");
    setting.transform = optional_value<std::string>(table, "transform");
    setting.scale = optional_value<float>(table, "scale");
    return setting;
}

void write_output_setting(toml::table & table, const ProfileOutputSetting & setting) {
    table.insert("output", setting.output);

    if (setting.on.has_value()) {
        table.insert("on", setting.on.value());
    }

    if (setting.mode.has_value()) {
        table.insert("mode", setting.mode.value());
    }
    if (setting.preferred || setting.preferred_explicit) {
        table.insert("preferred", setting.preferred);
    }
    if (setting.pos.has_value()) {
        table.insert("pos", setting.pos.value());
    }
    if (setting.left_of.has_value()) {
        table.insert("left_of", setting.left_of.value());
    }
    if (setting.right_of.has_value()) {
        table.insert("right_of", setting.right_of.value());
    }
    if (setting.above.has_value()) {
        table.insert("above", setting.above.value());
    }
    if (setting.below.has_value()) {
        table.insert("below", setting.below.value());
    }
    if (setting.transform.has_value()) {
        table.insert("transform", setting.transform.value());
    }
    if (setting.scale.has_value()) {
        table.insert("scale", setting.scale.value());
    }
    if (setting.adaptive_sync.has_value()) {
        table.insert("adaptive_sync", setting.adaptive_sync.value());
    }
}

}  // namespace

AutoWlrRandrConfigRepository::AutoWlrRandrConfigRepository(std::filesystem::path config_path)
    : config_path_(std::move(config_path)) {}

auto AutoWlrRandrConfigRepository::default_config_path() -> std::filesystem::path {
    const QString config_home = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    return std::filesystem::path(config_home.toStdString()) / "auto-wlr-randr" / "config.toml";
}

auto AutoWlrRandrConfigRepository::get_config_path() const -> const std::filesystem::path & {
    return config_path_;
}

auto AutoWlrRandrConfigRepository::load() const -> AutoWlrRandrConfig {
    AutoWlrRandrConfig config;
    config.path = config_path_.string();

    const auto file = read_config_file(config_path_);
    config.file_snapshot = file;
    if (!file.content.has_value()) {
        spdlog::info("auto-wlr-randr config not found at {}, using empty config", config.path);
        return config;
    }

    const auto parsed = toml::parse(*file.content, config_path_.string());

    check_keys(parsed, { "on_no_match_exec", "profile" }, "root");
    config.on_no_match_exec = string_array(parsed, "on_no_match_exec");
    const auto * profile_table = parsed["profile"].as_table();
    if (profile_table == nullptr) {
        throw std::runtime_error("TOML configuration requires a 'profile' table");
    }
    // toml++ stores tables by key; backend IndexMap uses declaration order as priority.
    std::vector<std::pair<std::string, const toml::table *>> profiles;
    for (const auto & [profile_id, node] : *profile_table) {
        const auto * value = node.as_table();
        if (value == nullptr) {
            throw std::runtime_error(
                "Expected TOML table for profile '" + std::string(profile_id.str()) + "'"
            );
        }
        profiles.emplace_back(std::string(profile_id.str()), value);
    }
    std::stable_sort(profiles.begin(), profiles.end(), [](const auto & left, const auto & right) {
        return left.second->source().begin < right.second->source().begin;
    });
    for (const auto & [id, value] : profiles) {
        check_keys(*value, { "exec", "settings" }, "profile." + id);
        AutoWlrRandrProfile profile;
        profile.id = id;
        profile.exec = string_array(*value, "exec");
        if (value->contains("settings")) {
            const auto * settings = (*value)["settings"].as_array();
            if (settings == nullptr) {
                throw std::runtime_error("Expected TOML array for profile '" + id + "' settings");
            }
            for (const auto & node : *settings) {
                const auto * setting = node.as_table();
                if (setting == nullptr) {
                    throw std::runtime_error(
                        "Expected TOML table in settings of profile '" + id + "'"
                    );
                }
                profile.settings.push_back(parse_output_setting(*setting));
            }
        }
        config.profiles.push_back(std::move(profile));
    }

    spdlog::info("Loaded {} auto-wlr-randr profiles from {}", config.profiles.size(), config.path);
    return config;
}

auto AutoWlrRandrConfigRepository::save(const AutoWlrRandrConfig & config) const
    -> ConfigFileSnapshot {
    toml::table root;
    std::ostringstream output;
    std::set<std::string> profile_ids;

    if (!config.on_no_match_exec.empty()) {
        toml::array on_no_match_exec_array;
        for (const auto & command : config.on_no_match_exec) {
            on_no_match_exec_array.push_back(command);
        }
        root.insert("on_no_match_exec", on_no_match_exec_array);
    }

    if (config.profiles.empty()) {
        root.insert("profile", toml::table{});
    }
    if (!root.empty()) {
        output << root << "\n\n";
    }
    for (const auto & profile : config.profiles) {
        if (!profile_ids.insert(profile.id).second) {
            throw std::runtime_error(
                "Duplicate TOML profile cannot be saved safely: " + profile.id
            );
        }
        toml::table profile_table;

        if (!profile.exec.empty()) {
            toml::array exec_array;
            for (const auto & command : profile.exec) {
                exec_array.push_back(command);
            }
            profile_table.insert("exec", exec_array);
        }

        if (!profile.settings.empty()) {
            toml::array settings_array;
            for (const auto & setting : profile.settings) {
                toml::table setting_table;
                write_output_setting(setting_table, setting);
                settings_array.push_back(setting_table);
            }
            profile_table.insert("settings", settings_array);
        }

        toml::table profiles_table;
        profiles_table.insert(profile.id, profile_table);
        toml::table fragment;
        fragment.insert("profile", profiles_table);
        output << fragment << "\n\n";
    }

    const auto saved = save_config_file(config_path_, output.str(), config.file_snapshot);
    spdlog::info(
        "Saved {} auto-wlr-randr profiles to {}", config.profiles.size(), config_path_.string()
    );
    return saved;
}
