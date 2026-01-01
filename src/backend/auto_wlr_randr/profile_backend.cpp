#include "backend/auto_wlr_randr/profile_backend.hpp"

#include <spdlog/spdlog.h>

#include <string>
#include <vector>

#include "backend/auto_wlr_randr/conversions.hpp"
#include "backend/auto_wlr_randr/daemon_client.hpp"
#include "backend/auto_wlr_randr/pattern_matcher.hpp"
#include "backend/auto_wlr_randr/snapshot.hpp"

AutoWlrRandrProfileBackend::AutoWlrRandrProfileBackend() {
    spdlog::debug("Initializing auto-wlr-randr profile backend");
}

auto AutoWlrRandrProfileBackend::capabilities() const -> profile::ProfileEditorCapabilities {
    profile::ProfileEditorCapabilities capabilities;
    capabilities.pattern_matching = true;
    capabilities.output_match_preview = true;
    capabilities.service_integration = true;
    capabilities.force_switch = true;
    capabilities.on_no_match_exec = true;
    capabilities.preferred_mode = true;
    capabilities.relative_positioning = true;
    capabilities.adaptive_sync = true;
    return capabilities;
}

auto AutoWlrRandrProfileBackend::load_config() const -> profile::ProfileDocument {
    return auto_wlr_randr_conversions::to_profile_document(repository_.load());
}

void AutoWlrRandrProfileBackend::save_config(const profile::ProfileDocument & config) const {
    const AutoWlrRandrConfig native_config =
        auto_wlr_randr_conversions::from_profile_document(config);
    repository_.save(native_config);
    if (AutoWlrRandrDaemonClient::is_running()) {
        AutoWlrRandrDaemonClient::reload();
    }
}

void AutoWlrRandrProfileBackend::add_profile(
    profile::ProfileDocument & config, profile::ProfileDefinition profile
) const {
    AutoWlrRandrConfig native_config = auto_wlr_randr_conversions::from_profile_document(config);
    AutoWlrRandrConfigRepository::add_profile(
        native_config, auto_wlr_randr_conversions::from_profile_definition(profile)
    );
    config = auto_wlr_randr_conversions::to_profile_document(native_config);
}

void AutoWlrRandrProfileBackend::delete_profile(
    profile::ProfileDocument & config, const std::string & profile_id
) const {
    AutoWlrRandrConfig native_config = auto_wlr_randr_conversions::from_profile_document(config);
    AutoWlrRandrConfigRepository::delete_profile(native_config, profile_id);
    config = auto_wlr_randr_conversions::to_profile_document(native_config);
}

void AutoWlrRandrProfileBackend::duplicate_profile(
    profile::ProfileDocument & config, const std::string & source_id, const std::string & new_id
) const {
    AutoWlrRandrConfig native_config = auto_wlr_randr_conversions::from_profile_document(config);
    AutoWlrRandrConfigRepository::duplicate_profile(native_config, source_id, new_id);
    config = auto_wlr_randr_conversions::to_profile_document(native_config);
}

auto AutoWlrRandrProfileBackend::create_profile_from_live(const std::string & profile_id) const
    -> profile::ProfileDefinition {
    return auto_wlr_randr_conversions::to_profile_definition(
        auto_wlr_randr_snapshot::create_profile_from_live_state(profile_id)
    );
}

auto AutoWlrRandrProfileBackend::get_connected_outputs() const
    -> std::vector<profile::ConnectedOutput> {
    return auto_wlr_randr_conversions::to_connected_outputs(
        AutoWlrRandrPatternMatcher::get_connected_outputs()
    );
}

auto AutoWlrRandrProfileBackend::get_match_warning(
    const profile::ProfileDefinition & profile,
    const std::vector<profile::ConnectedOutput> & connected_outputs
) const -> std::string {
    return AutoWlrRandrPatternMatcher::get_match_warning(
        auto_wlr_randr_conversions::from_profile_definition(profile),
        auto_wlr_randr_conversions::from_connected_outputs(connected_outputs)
    );
}

auto AutoWlrRandrProfileBackend::describe_output_match(
    const std::string & output_selector,
    const std::vector<profile::ConnectedOutput> & connected_outputs
) const -> std::string {
    const auto native_outputs =
        auto_wlr_randr_conversions::from_connected_outputs(connected_outputs);
    if (const auto matched =
            AutoWlrRandrPatternMatcher::find_matching_output(output_selector, native_outputs)) {
        return "Matches " + matched->display_label();
    }
    return "No connected output matches this pattern";
}

void AutoWlrRandrProfileBackend::reload_service() const {
    AutoWlrRandrDaemonClient::reload();
}

void AutoWlrRandrProfileBackend::switch_profile(const std::string & profile_id, bool force) const {
    AutoWlrRandrDaemonClient::switch_profile(profile_id, force);
}

auto AutoWlrRandrProfileBackend::service_status() const -> profile::ProfileServiceStatus {
    return auto_wlr_randr_conversions::to_service_status(AutoWlrRandrDaemonClient::status());
}
