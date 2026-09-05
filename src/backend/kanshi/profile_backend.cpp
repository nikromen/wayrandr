#include "backend/kanshi/profile_backend.hpp"

#include <fnmatch.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstddef>
#include <exception>
#include <string>
#include <vector>

#include "backend/kanshi/conversions.hpp"
#include "backend/kanshi/daemon_client.hpp"
#include "backend/kanshi/types.hpp"
#include "backend/profile/editor_backend.hpp"
#include "backend/profile/types.hpp"

namespace {

auto resolve_output_selector(
    const std::string & selector, const profile::ProfileDocument & document
) -> std::string {
    if (!selector.empty() && selector.front() == '$' && document.includes.empty()) {
        // Kanshi uses the first global definition of the alias. An unloaded include
        // may define it earlier, so main-file definitions alone are insufficient then.
        for (const auto & output : document.global_outputs) {
            if (output.alias == selector) {
                return output.output;
            }
        }
    }
    return selector;
}

auto unresolved_alias_warning(
    const std::string & selector, const profile::ProfileDocument & document
) -> std::string {
    if (selector.empty() || selector.front() != '$') {
        return {};
    }
    if (!document.includes.empty()) {
        return "Preview cannot be determined: includes may define output aliases.";
    }
    return "Undefined output alias: " + selector;
}

auto matches_output(const std::string & selector, const profile::ConnectedOutput & output) -> bool {
    // Kanshi 1.9.0 matches connector names literally and globs only the full identifier.
    const auto identifier = output.make.value_or("Unknown") + " " +
        output.model.value_or("Unknown") + " " + output.serial.value_or("Unknown");
    return selector == "*" || selector == output.name ||
        fnmatch(selector.c_str(), identifier.c_str(), 0) == 0;
}

}  // namespace

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
    const std::vector<profile::ConnectedOutput> & connected_outputs,
    const profile::ProfileDocument & document
) const -> std::string {
    auto outputs = profile.outputs;
    // Native order is regular outputs, wildcard outputs, then ...output (also
    // regular before wildcard). This sorting precedes alias resolution in Kanshi.
    std::stable_sort(outputs.begin(), outputs.end(), [](const auto & left, const auto & right) {
        if (left.multi_output != right.multi_output) {
            return right.multi_output;
        }
        return left.output.find('*') == std::string::npos &&
            right.output.find('*') != std::string::npos;
    });
    // Resolve every selector before matching: missing include context must not
    // become a definitive mismatch just because an earlier selector failed.
    for (auto & output : outputs) {
        output.output = resolve_output_selector(output.output, document);
        if (const auto warning = unresolved_alias_warning(output.output, document);
            !warning.empty()) {
            return warning;
        }
    }
    std::vector<bool> used(connected_outputs.size(), false);
    for (const auto & output : outputs) {
        bool matched = false;
        for (size_t index = 0; index < connected_outputs.size(); ++index) {
            if (!used[index] && matches_output(output.output, connected_outputs[index])) {
                used[index] = true;
                matched = true;
                if (!output.multi_output) {
                    break;
                }
            }
        }
        if (!matched && !output.multi_output) {
            return "Profile patterns do not match the currently connected outputs.";
        }
    }
    if (std::find(used.begin(), used.end(), false) != used.end()) {
        return "Profile patterns do not match the currently connected outputs.";
    }
    return {};
}

auto KanshiProfileBackend::describe_output_match(
    const std::string & output_selector,
    const std::vector<profile::ConnectedOutput> & connected_outputs,
    const profile::ProfileDocument & document
) const -> std::string {
    const auto selector = resolve_output_selector(output_selector, document);
    if (const auto warning = unresolved_alias_warning(selector, document); !warning.empty()) {
        return warning;
    }
    for (const auto & output : connected_outputs) {
        if (matches_output(selector, output)) {
            return "Matches " + output.display_label();
        }
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
