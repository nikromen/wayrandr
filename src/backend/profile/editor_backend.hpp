#pragma once

#include <string>
#include <vector>

#include "backend/profile/types.hpp"
#include "monitor_specs.hpp"
#include "utils/config_file.hpp"

struct ProfileSaveResult {
    ConfigFileSnapshot file_snapshot;
    bool daemon_reloaded = false;
    std::string reload_error;
};

class ProfileEditorBackend {
public:
    virtual ~ProfileEditorBackend() = default;

    [[nodiscard]] virtual auto capabilities() const -> profile::ProfileEditorCapabilities = 0;

    [[nodiscard]] virtual auto load_config() const -> profile::ProfileDocument = 0;
    // Write failures throw. Once saved, reload failures are reported separately.
    [[nodiscard]] virtual auto save_config(const profile::ProfileDocument & config) const
        -> ProfileSaveResult = 0;

    void add_profile(profile::ProfileDocument & config, profile::ProfileDefinition profile) const;
    void delete_profile(profile::ProfileDocument & config, const std::string & profile_id) const;
    void duplicate_profile(
        profile::ProfileDocument & config, const std::string & source_id, const std::string & new_id
    ) const;

    [[nodiscard]] auto create_profile_from_live(const std::string & profile_id) const
        -> profile::ProfileDefinition;
    [[nodiscard]] auto get_connected_outputs(const std::vector<MonitorSpecs> & monitors) const
        -> std::vector<profile::ConnectedOutput>;
    [[nodiscard]] virtual auto get_match_warning(
        const profile::ProfileDefinition & profile,
        const std::vector<profile::ConnectedOutput> & connected_outputs
    ) const -> std::string = 0;
    [[nodiscard]] virtual auto describe_output_match(
        const std::string & output_selector,
        const std::vector<profile::ConnectedOutput> & connected_outputs
    ) const -> std::string = 0;

    virtual void reload_service() const = 0;
    virtual void switch_profile(const std::string & profile_id, bool force = false) const = 0;
    [[nodiscard]] virtual auto service_status() const -> profile::ProfileServiceStatus = 0;
};
