#include "backend/kanshi/snapshot.hpp"

#include <cstddef>
#include <string>
#include <utility>

#include "backend/kanshi/types.hpp"
#include "monitor_specs.hpp"

namespace kanshi_snapshot {

auto create_profile_from_live_state(const std::string & profile_id) -> KanshiProfile {
    KanshiProfile profile;
    profile.id = profile_id;

    const auto monitors = get_monitor_specs_list();
    profile.outputs.reserve(monitors.size());

    for (const auto & monitor : monitors) {
        KanshiOutputSetting setting;
        if (const auto identifier = monitor.build_identifier()) {
            setting.criteria = identifier.value();
        } else {
            setting.criteria = monitor.get_name();
        }

        if (monitor.is_enabled() && monitor.get_enabled_monitor_settings().has_value()) {
            setting.enabled = true;
            const auto & enabled_settings = monitor.get_enabled_monitor_settings().value();
            const auto & modes = monitor.get_modes();
            const size_t active_mode_index = enabled_settings.get_active_mode_index();
            if (active_mode_index < modes.size()) {
                setting.mode = modes[active_mode_index].to_string();
            }

            const auto & position = enabled_settings.get_position();
            setting.position = std::to_string(position.x) + "," + std::to_string(position.y);
            setting.scale = enabled_settings.get_scale();
            setting.transform = transform_utils::to_string(enabled_settings.get_transform());
            setting.adaptive_sync = enabled_settings.is_adaptive_sync();
        } else {
            setting.enabled = false;
        }

        profile.outputs.push_back(std::move(setting));
    }

    return profile;
}

}  // namespace kanshi_snapshot
