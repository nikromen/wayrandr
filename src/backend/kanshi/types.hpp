#pragma once

#include <optional>
#include <string>
#include <vector>

#include "utils/config_file.hpp"

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
    std::optional<std::string> alias;
};

struct KanshiProfile {
    bool anonymous = false;
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
    ConfigFileSnapshot file_snapshot;
};

struct KanshiDaemonStatus {
    bool daemon_running = false;
    std::string active_profile;
    std::vector<std::string> connected_outputs;
};
