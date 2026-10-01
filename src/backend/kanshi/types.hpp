#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

struct KanshiOutputSetting {
    std::string criteria;
    bool multi_output = false;
    std::optional<bool> enabled;
    std::optional<std::string> mode;
    bool preferred = false;
    std::optional<std::string> position;
    std::optional<std::string> transform;
    std::optional<float> scale;
    std::optional<bool> adaptive_sync;
};

struct KanshiProfile {
    std::string id;
    std::vector<std::string> exec;
    std::vector<KanshiOutputSetting> outputs;
};

struct KanshiConfig {
    std::string path;
    std::vector<std::string> includes;
    std::vector<std::string> preserved_directives;
    std::vector<KanshiOutputSetting> global_outputs;
    std::vector<KanshiProfile> profiles;
};

struct KanshiConnectedOutputInfo {
    std::string name;
    std::optional<std::string> make;
    std::optional<std::string> model;
    std::optional<std::string> serial;

    [[nodiscard]] auto build_identifier() const -> std::optional<std::string>;
    [[nodiscard]] auto display_label() const -> std::string;
};

struct KanshiDaemonStatus {
    bool daemon_running = false;
    std::string active_profile;
    std::vector<std::string> connected_outputs;
};

namespace kanshi_profile_mode_utils {
[[nodiscard]] auto parse_mode_string(const std::string & mode)
    -> std::optional<std::pair<int, int>>;
[[nodiscard]] auto is_transform_rotated(const std::string & transform) -> bool;
[[nodiscard]] auto layout_width(const KanshiOutputSetting & setting) -> int;
[[nodiscard]] auto layout_height(const KanshiOutputSetting & setting) -> int;
[[nodiscard]] auto parse_position(const std::optional<std::string> & position)
    -> std::optional<std::pair<int, int>>;
}  // namespace kanshi_profile_mode_utils
