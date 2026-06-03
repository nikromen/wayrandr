#include "backend/auto_wlr_randr/profile_backend.hpp"

#include <spdlog/spdlog.h>

#include <cstddef>
#include <exception>
#include <string>
#include <vector>

#include "backend/auto_wlr_randr/conversions.hpp"
#include "backend/auto_wlr_randr/daemon_client.hpp"
#include "backend/auto_wlr_randr/types.hpp"
#include "backend/profile/editor_backend.hpp"
#include "backend/profile/matching.hpp"
#include "backend/profile/types.hpp"

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

auto AutoWlrRandrProfileBackend::save_config(const profile::ProfileDocument & config) const
    -> ProfileSaveResult {
    const AutoWlrRandrConfig native_config =
        auto_wlr_randr_conversions::from_profile_document(config);
    ProfileSaveResult result;
    result.file_snapshot = repository_.save(native_config);
    try {
        if (AutoWlrRandrDaemonClient::is_running()) {
            AutoWlrRandrDaemonClient::reload();
            result.daemon_reloaded = true;
        }
    } catch (const std::exception & error) {
        result.reload_error = error.what();
    }
    return result;
}

auto AutoWlrRandrProfileBackend::get_match_warning(
    const profile::ProfileDefinition & profile,
    const std::vector<profile::ConnectedOutput> & connected_outputs
) const -> std::string {
    if (profile.outputs.size() != connected_outputs.size()) {
        return profile::matching::get_match_warning(
            profile, connected_outputs, profile::matching::InvalidGlobBehavior::NO_MATCH
        );
    }

    // auto-wlr-randr 1.2.0 takes the first unused output without reassigning earlier matches.
    std::vector<bool> used(connected_outputs.size(), false);
    for (const auto & output : profile.outputs) {
        size_t index = 0;
        while (index < connected_outputs.size() &&
               (used[index] ||
                !profile::matching::matches_pattern(
                    output.output,
                    connected_outputs[index],
                    profile::matching::InvalidGlobBehavior::NO_MATCH
                ))) {
            ++index;
        }
        if (index == connected_outputs.size()) {
            return "Profile patterns do not match the currently connected outputs.";
        }
        used[index] = true;
    }
    return {};
}

auto AutoWlrRandrProfileBackend::describe_output_match(
    const std::string & output_selector,
    const std::vector<profile::ConnectedOutput> & connected_outputs
) const -> std::string {
    if (const auto matched = profile::matching::find_matching_output(
            output_selector, connected_outputs, profile::matching::InvalidGlobBehavior::NO_MATCH
        )) {
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
