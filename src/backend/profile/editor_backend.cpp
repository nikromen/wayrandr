#include "backend/profile/editor_backend.hpp"

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "backend/profile/types.hpp"
#include "monitor_specs.hpp"

void ProfileEditorBackend::add_profile(
    profile::ProfileDocument & config, profile::ProfileDefinition profile
) const {
    const auto it =
        std::find_if(config.profiles.begin(), config.profiles.end(), [&](const auto & existing) {
            return existing.id == profile.id;
        });
    if (it != config.profiles.end()) {
        *it = std::move(profile);
        return;
    }
    config.profiles.push_back(std::move(profile));
}

void ProfileEditorBackend::delete_profile(
    profile::ProfileDocument & config, const std::string & profile_id
) const {
    config.profiles.erase(
        std::remove_if(
            config.profiles.begin(),
            config.profiles.end(),
            [&](const auto & profile) { return profile.id == profile_id; }
        ),
        config.profiles.end()
    );
}

void ProfileEditorBackend::duplicate_profile(
    profile::ProfileDocument & config, const std::string & source_id, const std::string & new_id
) const {
    const auto it =
        std::find_if(config.profiles.begin(), config.profiles.end(), [&](const auto & profile) {
            return profile.id == source_id;
        });
    if (it == config.profiles.end()) {
        throw std::invalid_argument("Source profile not found: " + source_id);
    }
    auto copy = *it;
    copy.id = new_id;
    copy.anonymous = false;
    add_profile(config, std::move(copy));
}

auto ProfileEditorBackend::create_profile_from_live(const std::string & profile_id) const
    -> profile::ProfileDefinition {
    profile::ProfileDefinition profile;
    profile.id = profile_id;

    const auto monitors = get_monitor_specs_list();
    profile.outputs.reserve(monitors.size());
    for (const auto & monitor : monitors) {
        profile::ProfileOutputDefinition output;
        if (const auto identifier = monitor.build_identifier()) {
            output.output = identifier.value();
        } else {
            output.output = monitor.get_name();
        }

        if (monitor.is_enabled() && monitor.get_enabled_monitor_settings().has_value()) {
            output.enabled = true;
            const auto & settings = monitor.get_enabled_monitor_settings().value();
            const auto & modes = monitor.get_modes();
            const size_t active_mode_index = settings.get_active_mode_index();
            if (active_mode_index < modes.size()) {
                output.mode = modes[active_mode_index].to_string();
            }
            const auto & position = settings.get_position();
            output.pos = std::to_string(position.x) + "," + std::to_string(position.y);
            output.scale = settings.get_scale();
            output.transform = transform_utils::to_string(settings.get_transform());
            output.adaptive_sync = settings.is_adaptive_sync();
        } else {
            output.enabled = false;
        }
        profile.outputs.push_back(std::move(output));
    }
    return profile;
}

auto ProfileEditorBackend::get_connected_outputs(const std::vector<MonitorSpecs> & monitors) const
    -> std::vector<profile::ConnectedOutput> {
    std::vector<profile::ConnectedOutput> outputs;
    outputs.reserve(monitors.size());
    for (const auto & monitor : monitors) {
        profile::ConnectedOutput output;
        output.name = monitor.get_name();
        output.make = monitor.get_make();
        output.model = monitor.get_model();
        output.serial = monitor.get_serial_number();
        for (auto * value : { &output.make, &output.model, &output.serial }) {
            if (value->has_value() && value->value().empty()) {
                value->reset();
            }
        }
        outputs.push_back(std::move(output));
    }
    return outputs;
}
