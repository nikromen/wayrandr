#pragma once

#include <string>
#include <vector>

#include "backend/profile/types.hpp"

class ProfileEditorBackend {
public:
    virtual ~ProfileEditorBackend() = default;

    [[nodiscard]] virtual auto capabilities() const -> profile::ProfileEditorCapabilities = 0;

    [[nodiscard]] virtual auto load_config() const -> profile::ProfileDocument = 0;
    virtual void save_config(const profile::ProfileDocument & config) const = 0;

    virtual void add_profile(
        profile::ProfileDocument & config, profile::ProfileDefinition profile
    ) const = 0;
    virtual void delete_profile(
        profile::ProfileDocument & config, const std::string & profile_id
    ) const = 0;
    virtual void duplicate_profile(
        profile::ProfileDocument & config, const std::string & source_id, const std::string & new_id
    ) const = 0;

    [[nodiscard]] virtual auto create_profile_from_live(const std::string & profile_id) const
        -> profile::ProfileDefinition = 0;
    [[nodiscard]] virtual auto get_connected_outputs() const
        -> std::vector<profile::ConnectedOutput> = 0;
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
