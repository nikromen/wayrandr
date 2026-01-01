#include "backend/auto_wlr_randr/config_repository.hpp"

#include <spdlog/spdlog.h>
#include <toml++/toml.h>

#include <QStandardPaths>
#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <utility>

namespace {

auto parse_output_setting(const toml::table & table) -> ProfileOutputSetting {
    ProfileOutputSetting setting;
    if (auto output = table["output"].value<std::string>()) {
        setting.output = *output;
    }

    if (table.contains("on")) {
        setting.on = table["on"].value<bool>();
    }
    setting.preferred = table["preferred"].value_or(false);
    if (table.contains("adaptive_sync")) {
        setting.adaptive_sync = table["adaptive_sync"].value<bool>();
    }

    if (auto mode = table["mode"].value<std::string>()) {
        setting.mode = *mode;
    }
    if (auto pos = table["pos"].value<std::string>()) {
        setting.pos = *pos;
    }
    if (auto left_of = table["left_of"].value<std::string>()) {
        setting.left_of = *left_of;
    }
    if (auto right_of = table["right_of"].value<std::string>()) {
        setting.right_of = *right_of;
    }
    if (auto above = table["above"].value<std::string>()) {
        setting.above = *above;
    }
    if (auto below = table["below"].value<std::string>()) {
        setting.below = *below;
    }
    if (auto transform = table["transform"].value<std::string>()) {
        setting.transform = *transform;
    }
    if (auto scale = table["scale"].value<float>()) {
        setting.scale = *scale;
    }

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
    if (setting.preferred) {
        table.insert("preferred", true);
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

    if (!std::filesystem::exists(config_path_)) {
        spdlog::info("auto-wlr-randr config not found at {}, using empty config", config.path);
        return config;
    }

    const auto parsed = toml::parse_file(config_path_.string());

    if (const auto * on_no_match_exec = parsed["on_no_match_exec"].as_array()) {
        for (const auto & command : *on_no_match_exec) {
            if (auto command_value = command.value<std::string>()) {
                config.on_no_match_exec.push_back(*command_value);
            }
        }
    }

    if (const auto * profile_table = parsed["profile"].as_table()) {
        for (const auto & [profile_id, profile_node] : *profile_table) {
            const auto * profile_value = profile_node.as_table();
            if (profile_value == nullptr) {
                continue;
            }

            AutoWlrRandrProfile profile;
            profile.id = std::string(profile_id.str());

            if (const auto * exec_array = profile_value->get_as<toml::array>("exec")) {
                for (const auto & command : *exec_array) {
                    if (auto command_value = command.value<std::string>()) {
                        profile.exec.push_back(*command_value);
                    }
                }
            }

            if (const auto * settings_array = profile_value->get_as<toml::array>("settings")) {
                for (const auto & setting_node : *settings_array) {
                    if (const auto * setting_table = setting_node.as_table()) {
                        profile.settings.push_back(parse_output_setting(*setting_table));
                    }
                }
            }

            config.profiles.push_back(std::move(profile));
        }
    }

    spdlog::info("Loaded {} auto-wlr-randr profiles from {}", config.profiles.size(), config.path);
    return config;
}

void AutoWlrRandrConfigRepository::save(const AutoWlrRandrConfig & config) const {
    if (const auto parent = config_path_.parent_path(); !parent.empty()) {
        std::filesystem::create_directories(parent);
    }

    toml::table root;
    toml::table profiles_table;

    if (!config.on_no_match_exec.empty()) {
        toml::array on_no_match_exec_array;
        for (const auto & command : config.on_no_match_exec) {
            on_no_match_exec_array.push_back(command);
        }
        root.insert("on_no_match_exec", on_no_match_exec_array);
    }

    for (const auto & profile : config.profiles) {
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

        profiles_table.insert(profile.id, profile_table);
    }

    root.insert("profile", profiles_table);

    std::ofstream output(config_path_);
    if (!output) {
        throw std::runtime_error("Failed to open config file for writing: " + config.path);
    }

    output << root;
    spdlog::info("Saved {} auto-wlr-randr profiles to {}", config.profiles.size(), config.path);
}

void AutoWlrRandrConfigRepository::add_profile(
    AutoWlrRandrConfig & config, AutoWlrRandrProfile profile
) {
    const auto it = std::find_if(
        config.profiles.begin(), config.profiles.end(), [&](const AutoWlrRandrProfile & existing) {
            return existing.id == profile.id;
        }
    );
    if (it != config.profiles.end()) {
        *it = std::move(profile);
        return;
    }

    config.profiles.push_back(std::move(profile));
}

void AutoWlrRandrConfigRepository::delete_profile(
    AutoWlrRandrConfig & config, const std::string & profile_id
) {
    config.profiles.erase(
        std::remove_if(
            config.profiles.begin(),
            config.profiles.end(),
            [&](const AutoWlrRandrProfile & profile) { return profile.id == profile_id; }
        ),
        config.profiles.end()
    );
}

void AutoWlrRandrConfigRepository::duplicate_profile(
    AutoWlrRandrConfig & config, const std::string & source_id, const std::string & new_id
) {
    const auto it = std::find_if(
        config.profiles.begin(), config.profiles.end(), [&](const AutoWlrRandrProfile & profile) {
            return profile.id == source_id;
        }
    );
    if (it == config.profiles.end()) {
        throw std::invalid_argument("Source profile not found: " + source_id);
    }

    AutoWlrRandrProfile copy = *it;
    copy.id = new_id;
    add_profile(config, std::move(copy));
}
