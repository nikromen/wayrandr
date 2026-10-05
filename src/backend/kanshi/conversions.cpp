#include "backend/kanshi/conversions.hpp"

#include <vector>

#include "backend/kanshi/types.hpp"
#include "backend/profile/types.hpp"

namespace kanshi_conversions {

auto to_profile_output(const KanshiOutputSetting & setting) -> profile::ProfileOutputDefinition {
    profile::ProfileOutputDefinition output;
    output.output = setting.criteria;
    output.multi_output = setting.multi_output;
    output.alias = setting.alias;
    output.enabled = setting.enabled;
    output.mode = setting.mode;
    output.preferred = setting.preferred;
    output.pos = setting.position;
    output.transform = setting.transform;
    output.scale = setting.scale;
    output.adaptive_sync = setting.adaptive_sync;
    return output;
}

auto from_profile_output(const profile::ProfileOutputDefinition & output) -> KanshiOutputSetting {
    KanshiOutputSetting setting;
    setting.criteria = output.output;
    setting.multi_output = output.multi_output;
    setting.alias = output.alias;
    setting.enabled = output.enabled;
    setting.mode = output.mode;
    setting.preferred = output.preferred;
    setting.position = output.pos;
    setting.transform = output.transform;
    setting.scale = output.scale;
    setting.adaptive_sync = output.adaptive_sync;
    return setting;
}

auto to_profile_definition(const KanshiProfile & profile) -> profile::ProfileDefinition {
    profile::ProfileDefinition definition;
    definition.id = profile.id;
    definition.anonymous = profile.anonymous;
    definition.exec = profile.exec;
    for (const auto & setting : profile.outputs) {
        definition.outputs.push_back(to_profile_output(setting));
    }
    return definition;
}

auto from_profile_definition(const profile::ProfileDefinition & profile) -> KanshiProfile {
    KanshiProfile definition;
    definition.id = profile.id;
    definition.anonymous = profile.anonymous;
    definition.exec = profile.exec;
    for (const auto & output : profile.outputs) {
        definition.outputs.push_back(from_profile_output(output));
    }
    return definition;
}

auto to_profile_document(const KanshiConfig & config) -> profile::ProfileDocument {
    profile::ProfileDocument document;
    document.path = config.path;
    document.file_snapshot = config.file_snapshot;
    document.includes = config.includes;
    for (const auto & output : config.global_outputs) {
        document.global_outputs.push_back(to_profile_output(output));
    }
    for (const auto & profile : config.profiles) {
        document.profiles.push_back(to_profile_definition(profile));
    }
    return document;
}

auto from_profile_document(const profile::ProfileDocument & document) -> KanshiConfig {
    KanshiConfig config;
    config.path = document.path;
    config.file_snapshot = document.file_snapshot;
    config.includes = document.includes;
    for (const auto & output : document.global_outputs) {
        config.global_outputs.push_back(from_profile_output(output));
    }
    for (const auto & profile : document.profiles) {
        config.profiles.push_back(from_profile_definition(profile));
    }
    return config;
}

auto to_connected_output(const KanshiConnectedOutputInfo & output) -> profile::ConnectedOutput {
    profile::ConnectedOutput connected;
    connected.name = output.name;
    connected.make = output.make;
    connected.model = output.model;
    connected.serial = output.serial;
    return connected;
}

auto from_connected_output(const profile::ConnectedOutput & output) -> KanshiConnectedOutputInfo {
    KanshiConnectedOutputInfo connected;
    connected.name = output.name;
    connected.make = output.make;
    connected.model = output.model;
    connected.serial = output.serial;
    return connected;
}

auto to_service_status(const KanshiDaemonStatus & status) -> profile::ProfileServiceStatus {
    profile::ProfileServiceStatus service_status;
    service_status.service_running = status.daemon_running;
    service_status.active_profile = status.active_profile;
    service_status.connected_outputs = status.connected_outputs;
    return service_status;
}

auto to_connected_outputs(const std::vector<KanshiConnectedOutputInfo> & outputs)
    -> std::vector<profile::ConnectedOutput> {
    std::vector<profile::ConnectedOutput> converted;
    converted.reserve(outputs.size());
    for (const auto & output : outputs) {
        converted.push_back(to_connected_output(output));
    }
    return converted;
}

auto from_connected_outputs(const std::vector<profile::ConnectedOutput> & outputs)
    -> std::vector<KanshiConnectedOutputInfo> {
    std::vector<KanshiConnectedOutputInfo> converted;
    converted.reserve(outputs.size());
    for (const auto & output : outputs) {
        converted.push_back(from_connected_output(output));
    }
    return converted;
}

}  // namespace kanshi_conversions
