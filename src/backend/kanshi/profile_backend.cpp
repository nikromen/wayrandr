#include "backend/kanshi/profile_backend.hpp"

#include <spdlog/spdlog.h>

#include <string>
#include <vector>

#include "backend/kanshi/conversions.hpp"
#include "backend/kanshi/daemon_client.hpp"
#include "backend/kanshi/pattern_matcher.hpp"
#include "backend/kanshi/snapshot.hpp"
#include "backend/kanshi/types.hpp"

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

void KanshiProfileBackend::save_config(const profile::ProfileDocument & config) const {
    KanshiConfig native_config = kanshi_conversions::from_profile_document(config);
    const KanshiConfig existing = repository_.load();
    native_config.path = repository_.get_config_path().string();
    native_config.includes = existing.includes;
    native_config.preserved_directives = existing.preserved_directives;
    native_config.global_outputs = existing.global_outputs;
    repository_.save(native_config);
    if (KanshiDaemonClient::is_running()) {
        KanshiDaemonClient::reload();
    }
}

void KanshiProfileBackend::add_profile(
    profile::ProfileDocument & config, profile::ProfileDefinition profile
) const {
    KanshiConfig native_config = kanshi_conversions::from_profile_document(config);
    KanshiConfigRepository::add_profile(
        native_config, kanshi_conversions::from_profile_definition(profile)
    );
    config = kanshi_conversions::to_profile_document(native_config);
}

void KanshiProfileBackend::delete_profile(
    profile::ProfileDocument & config, const std::string & profile_id
) const {
    KanshiConfig native_config = kanshi_conversions::from_profile_document(config);
    KanshiConfigRepository::delete_profile(native_config, profile_id);
    config = kanshi_conversions::to_profile_document(native_config);
}

void KanshiProfileBackend::duplicate_profile(
    profile::ProfileDocument & config, const std::string & source_id, const std::string & new_id
) const {
    KanshiConfig native_config = kanshi_conversions::from_profile_document(config);
    KanshiConfigRepository::duplicate_profile(native_config, source_id, new_id);
    config = kanshi_conversions::to_profile_document(native_config);
}

auto KanshiProfileBackend::create_profile_from_live(const std::string & profile_id) const
    -> profile::ProfileDefinition {
    return kanshi_conversions::to_profile_definition(
        kanshi_snapshot::create_profile_from_live_state(profile_id)
    );
}

auto KanshiProfileBackend::get_connected_outputs() const -> std::vector<profile::ConnectedOutput> {
    return kanshi_conversions::to_connected_outputs(KanshiPatternMatcher::get_connected_outputs());
}

auto KanshiProfileBackend::get_match_warning(
    const profile::ProfileDefinition & profile,
    const std::vector<profile::ConnectedOutput> & connected_outputs
) const -> std::string {
    return KanshiPatternMatcher::get_match_warning(
        kanshi_conversions::from_profile_definition(profile),
        kanshi_conversions::from_connected_outputs(connected_outputs)
    );
}

auto KanshiProfileBackend::describe_output_match(
    const std::string & output_selector,
    const std::vector<profile::ConnectedOutput> & connected_outputs
) const -> std::string {
    const auto native_outputs = kanshi_conversions::from_connected_outputs(connected_outputs);
    if (const auto matched =
            KanshiPatternMatcher::find_matching_output(output_selector, native_outputs)) {
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
