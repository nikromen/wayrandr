#include "backend/kanshi/profile_backend.hpp"

#include <spdlog/spdlog.h>

#include <exception>
#include <string>
#include <vector>

#include "backend/kanshi/conversions.hpp"
#include "backend/kanshi/daemon_client.hpp"
#include "backend/kanshi/types.hpp"
#include "backend/profile/editor_backend.hpp"
#include "backend/profile/matching.hpp"
#include "backend/profile/types.hpp"

KanshiProfileBackend::KanshiProfileBackend() {
    spdlog::debug("Initializing kanshi profile backend");
}

auto KanshiProfileBackend::capabilities() const -> profile::ProfileEditorCapabilities {
    profile::ProfileEditorCapabilities capabilities;
    capabilities.pattern_matching = true;
    capabilities.output_match_preview = true;
    capabilities.service_integration = true;
    capabilities.force_switch = false;
    capabilities.on_no_match_exec = false;
    capabilities.preferred_mode = true;
    capabilities.relative_positioning = false;
    capabilities.adaptive_sync = true;
    return capabilities;
}

auto KanshiProfileBackend::load_config() const -> profile::ProfileDocument {
    return kanshi_conversions::to_profile_document(repository_.load());
}

auto KanshiProfileBackend::save_config(const profile::ProfileDocument & config) const
    -> ProfileSaveResult {
    KanshiConfig native_config = kanshi_conversions::from_profile_document(config);
    native_config.path = repository_.get_config_path().string();
    ProfileSaveResult result;
    result.file_snapshot = repository_.save(native_config);
    try {
        if (KanshiDaemonClient::is_running()) {
            KanshiDaemonClient::reload();
            result.daemon_reloaded = true;
        }
    } catch (const std::exception & error) {
        result.reload_error = error.what();
    }
    return result;
}

auto KanshiProfileBackend::get_match_warning(
    const profile::ProfileDefinition & profile,
    const std::vector<profile::ConnectedOutput> & connected_outputs
) const -> std::string {
    return profile::matching::get_match_warning(
        profile, connected_outputs, profile::matching::InvalidGlobBehavior::LITERAL_NAME
    );
}

auto KanshiProfileBackend::describe_output_match(
    const std::string & output_selector,
    const std::vector<profile::ConnectedOutput> & connected_outputs
) const -> std::string {
    if (const auto matched = profile::matching::find_matching_output(
            output_selector, connected_outputs, profile::matching::InvalidGlobBehavior::LITERAL_NAME
        )) {
        return "Matches " + matched->display_label();
    }
    return "No connected output matches this pattern";
}

void KanshiProfileBackend::reload_service() const {
    KanshiDaemonClient::reload();
}

void KanshiProfileBackend::switch_profile(const std::string & profile_id, bool /*force*/) const {
    KanshiDaemonClient::switch_profile(profile_id);
}

auto KanshiProfileBackend::service_status() const -> profile::ProfileServiceStatus {
    return kanshi_conversions::to_service_status(KanshiDaemonClient::status());
}
