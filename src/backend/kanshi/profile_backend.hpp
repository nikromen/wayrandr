#pragma once

#include <string>
#include <vector>

#include "backend/profile/editor_backend.hpp"

class KanshiProfileBackend final : public ProfileEditorBackend {
public:
    KanshiProfileBackend() = default;
    ~KanshiProfileBackend() override = default;

    [[nodiscard]] auto capabilities() const -> profile::ProfileEditorCapabilities override;

    [[nodiscard]] auto load_config() const -> profile::ProfileDocument override;
    void save_config(const profile::ProfileDocument & config) const override;

    void add_profile(
        profile::ProfileDocument & config, profile::ProfileDefinition profile
    ) const override;
    void delete_profile(
        profile::ProfileDocument & config, const std::string & profile_id
    ) const override;
    void duplicate_profile(
        profile::ProfileDocument & config, const std::string & source_id, const std::string & new_id
    ) const override;

    [[nodiscard]] auto create_profile_from_live(const std::string & profile_id) const
        -> profile::ProfileDefinition override;
    [[nodiscard]] auto get_connected_outputs() const
        -> std::vector<profile::ConnectedOutput> override;
    [[nodiscard]] auto get_match_warning(
        const profile::ProfileDefinition & profile,
        const std::vector<profile::ConnectedOutput> & connected_outputs
    ) const -> std::string override;
    [[nodiscard]] auto describe_output_match(
        const std::string & output_selector,
        const std::vector<profile::ConnectedOutput> & connected_outputs
    ) const -> std::string override;

    void reload_service() const override;
    void switch_profile(const std::string & profile_id, bool force = false) const override;
    [[nodiscard]] auto service_status() const -> profile::ProfileServiceStatus override;
};
