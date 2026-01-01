#include "backend/kanshi/profile_backend.hpp"

#include <stdexcept>
#include <string>
#include <vector>

auto KanshiProfileBackend::capabilities() const -> profile::ProfileEditorCapabilities {
    profile::ProfileEditorCapabilities capabilities;
    capabilities.pattern_matching = false;
    capabilities.output_match_preview = false;
    capabilities.service_integration = true;
    capabilities.force_switch = false;
    capabilities.on_no_match_exec = false;
    capabilities.preferred_mode = false;
    capabilities.relative_positioning = true;
    capabilities.adaptive_sync = true;
    return capabilities;
}

auto KanshiProfileBackend::load_config() const -> profile::ProfileDocument {
    throw std::runtime_error("Kanshi profile editor is not implemented yet");
}

void KanshiProfileBackend::save_config(const profile::ProfileDocument & /*config*/) const {
    throw std::runtime_error("Kanshi profile editor is not implemented yet");
}

void KanshiProfileBackend::add_profile(
    profile::ProfileDocument & /*config*/, profile::ProfileDefinition /*profile*/
) const {
    throw std::runtime_error("Kanshi profile editor is not implemented yet");
}

void KanshiProfileBackend::delete_profile(
    profile::ProfileDocument & /*config*/, const std::string & /*profile_id*/
) const {
    throw std::runtime_error("Kanshi profile editor is not implemented yet");
}

void KanshiProfileBackend::duplicate_profile(
    profile::ProfileDocument & /*config*/,
    const std::string & /*source_id*/,
    const std::string & /*new_id*/
) const {
    throw std::runtime_error("Kanshi profile editor is not implemented yet");
}

auto KanshiProfileBackend::create_profile_from_live(const std::string & /*profile_id*/) const
    -> profile::ProfileDefinition {
    throw std::runtime_error("Kanshi profile editor is not implemented yet");
}

auto KanshiProfileBackend::get_connected_outputs() const -> std::vector<profile::ConnectedOutput> {
    return {};
}

auto KanshiProfileBackend::get_match_warning(
    const profile::ProfileDefinition & /*profile*/,
    const std::vector<profile::ConnectedOutput> & /*connected_outputs*/
) const -> std::string {
    return {};
}

auto KanshiProfileBackend::describe_output_match(
    const std::string & /*output_selector*/,
    const std::vector<profile::ConnectedOutput> & /*connected_outputs*/
) const -> std::string {
    return {};
}

void KanshiProfileBackend::reload_service() const {
    throw std::runtime_error("Kanshi profile editor is not implemented yet");
}

void KanshiProfileBackend::
    switch_profile(const std::string & /*profile_id*/, bool /*force*/) const {
    throw std::runtime_error("Kanshi profile editor is not implemented yet");
}

auto KanshiProfileBackend::service_status() const -> profile::ProfileServiceStatus {
    return {};
}
