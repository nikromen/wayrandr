#include "backend/auto_wlr_randr/conversions.hpp"

#include <vector>

namespace auto_wlr_randr_conversions {

auto to_profile_output(const ProfileOutputSetting & setting) -> profile::ProfileOutputDefinition {
    profile::ProfileOutputDefinition output;
    output.output = setting.output;
    output.enabled = setting.on;
    output.mode = setting.mode;
    output.preferred = setting.preferred;
    output.pos = setting.pos;
    output.left_of = setting.left_of;
    output.right_of = setting.right_of;
    output.above = setting.above;
    output.below = setting.below;
    output.transform = setting.transform;
    output.scale = setting.scale;
    output.adaptive_sync = setting.adaptive_sync;
    return output;
}

auto from_profile_output(const profile::ProfileOutputDefinition & output) -> ProfileOutputSetting {
    ProfileOutputSetting setting;
    setting.output = output.output;
    setting.on = output.enabled;
    setting.mode = output.mode;
    setting.preferred = output.preferred;
    setting.pos = output.pos;
    setting.left_of = output.left_of;
    setting.right_of = output.right_of;
    setting.above = output.above;
    setting.below = output.below;
    setting.transform = output.transform;
    setting.scale = output.scale;
    setting.adaptive_sync = output.adaptive_sync;
    return setting;
}

auto to_profile_definition(const AutoWlrRandrProfile & profile) -> profile::ProfileDefinition {
    profile::ProfileDefinition definition;
    definition.id = profile.id;
    definition.exec = profile.exec;
    for (const auto & setting : profile.settings) {
        definition.outputs.push_back(to_profile_output(setting));
    }
    return definition;
}

auto from_profile_definition(const profile::ProfileDefinition & profile) -> AutoWlrRandrProfile {
    AutoWlrRandrProfile definition;
    definition.id = profile.id;
    definition.exec = profile.exec;
    for (const auto & output : profile.outputs) {
        definition.settings.push_back(from_profile_output(output));
    }
    return definition;
}

auto to_profile_document(const AutoWlrRandrConfig & config) -> profile::ProfileDocument {
    profile::ProfileDocument document;
    document.path = config.path;
    document.on_no_match_exec = config.on_no_match_exec;
    for (const auto & profile : config.profiles) {
        document.profiles.push_back(to_profile_definition(profile));
    }
    return document;
}

auto from_profile_document(const profile::ProfileDocument & document) -> AutoWlrRandrConfig {
    AutoWlrRandrConfig config;
    config.path = document.path;
    config.on_no_match_exec = document.on_no_match_exec;
    for (const auto & profile : document.profiles) {
        config.profiles.push_back(from_profile_definition(profile));
    }
    return config;
}

auto to_connected_output(const ConnectedOutputInfo & output) -> profile::ConnectedOutput {
    profile::ConnectedOutput connected;
    connected.name = output.name;
    connected.make = output.make;
    connected.model = output.model;
    connected.serial = output.serial;
    return connected;
}

auto from_connected_output(const profile::ConnectedOutput & output) -> ConnectedOutputInfo {
    ConnectedOutputInfo connected;
    connected.name = output.name;
    connected.make = output.make;
    connected.model = output.model;
    connected.serial = output.serial;
    return connected;
}

auto to_service_status(const DaemonStatus & status) -> profile::ProfileServiceStatus {
    profile::ProfileServiceStatus service_status;
    service_status.service_running = status.daemon_running;
    service_status.active_profile = status.active_profile;
    service_status.connected_outputs = status.connected_outputs;
    return service_status;
}

auto to_connected_outputs(const std::vector<ConnectedOutputInfo> & outputs)
    -> std::vector<profile::ConnectedOutput> {
    std::vector<profile::ConnectedOutput> converted;
    converted.reserve(outputs.size());
    for (const auto & output : outputs) {
        converted.push_back(to_connected_output(output));
    }
    return converted;
}

auto from_connected_outputs(const std::vector<profile::ConnectedOutput> & outputs)
    -> std::vector<ConnectedOutputInfo> {
    std::vector<ConnectedOutputInfo> converted;
    converted.reserve(outputs.size());
    for (const auto & output : outputs) {
        converted.push_back(from_connected_output(output));
    }
    return converted;
}

}  // namespace auto_wlr_randr_conversions
