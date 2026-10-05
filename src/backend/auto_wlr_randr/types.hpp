#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

// Mirrors auto-wlr-randr 1.2.0 src/config.rs OutputSetting and Profile.

#include "utils/config_file.hpp"

struct ProfileOutputSetting {
    std::string output;
    std::optional<bool> on;
    std::optional<std::string> mode;
    bool preferred = false;
    std::optional<std::string> pos;
    std::optional<std::string> left_of;
    std::optional<std::string> right_of;
    std::optional<std::string> above;
    std::optional<std::string> below;
    std::optional<std::string> transform;
    std::optional<float> scale;
    std::optional<bool> adaptive_sync;
};

struct AutoWlrRandrProfile {
    std::string id;
    std::vector<std::string> exec;
    std::vector<ProfileOutputSetting> settings;
};

struct AutoWlrRandrConfig {
    std::string path;
    std::vector<std::string> on_no_match_exec;
    std::vector<AutoWlrRandrProfile> profiles;
    ConfigFileSnapshot file_snapshot;
};

struct ConnectedOutputInfo {
    std::string name;
    std::optional<std::string> make;
    std::optional<std::string> model;
    std::optional<std::string> serial;

    [[nodiscard]] auto build_identifier() const -> std::optional<std::string>;
    [[nodiscard]] auto display_label() const -> std::string;
};

struct DaemonStatus {
    bool daemon_running = false;
    std::string active_profile;
    std::vector<std::string> connected_outputs;
};

namespace profile_mode_utils {
[[nodiscard]] auto parse_mode_string(const std::string & mode)
    -> std::optional<std::pair<int, int>>;
[[nodiscard]] auto is_transform_rotated(const std::string & transform) -> bool;
[[nodiscard]] auto layout_width(const ProfileOutputSetting & setting) -> int;
[[nodiscard]] auto layout_height(const ProfileOutputSetting & setting) -> int;
[[nodiscard]] auto parse_position(const std::optional<std::string> & pos)
    -> std::optional<std::pair<int, int>>;
}  // namespace profile_mode_utils
