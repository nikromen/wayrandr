#pragma once

#include <optional>
#include <string>
#include <vector>

#include "utils/config_file.hpp"

namespace profile {

struct ProfileOutputDefinition {
    std::string output;
    bool multi_output = false;
    std::optional<std::string> alias;
    std::optional<bool> enabled;
    std::optional<std::string> mode;
    bool preferred = false;
    bool preferred_explicit = false;
    std::optional<std::string> pos;
    std::optional<std::string> left_of;
    std::optional<std::string> right_of;
    std::optional<std::string> above;
    std::optional<std::string> below;
    std::optional<std::string> transform;
    std::optional<float> scale;
    std::optional<bool> adaptive_sync;
};

struct ProfileDefinition {
    bool anonymous = false;
    std::string id;
    std::vector<std::string> exec;
    std::vector<ProfileOutputDefinition> outputs;
};

struct ProfileDocument {
    // Main-file Kanshi defaults/includes; included files are never expanded or edited.
    std::vector<std::string> includes;
    std::vector<ProfileOutputDefinition> global_outputs;
    std::string path;
    std::vector<std::string> on_no_match_exec;
    std::vector<ProfileDefinition> profiles;
    ConfigFileSnapshot file_snapshot;
};

struct ConnectedOutput {
    std::string name;
    std::optional<std::string> make;
    std::optional<std::string> model;
    std::optional<std::string> serial;

    [[nodiscard]] auto build_identifier() const -> std::optional<std::string>;
    [[nodiscard]] auto display_label() const -> std::string;
};

struct ProfileServiceStatus {
    bool service_running = false;
    std::string active_profile;
    std::vector<std::string> connected_outputs;
};

struct ProfileEditorCapabilities {
    bool pattern_matching = false;
    bool output_match_preview = false;
    bool service_integration = false;
    bool force_switch = false;
    bool on_no_match_exec = false;
    bool preferred_mode = false;
    bool relative_positioning = false;
    bool adaptive_sync = false;
};

}  // namespace profile
