#include "monitor_specs.hpp"

#include <spdlog/spdlog.h>

#include <cstdio>
#include <algorithm>
#include <cstddef>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "nlohmann/json_fwd.hpp"
#include "utils/helpers.hpp"

using json = nlohmann::json;

auto transform_utils::from_string(const std::string & str) -> Transform {
    if (str == "normal") {
        return Transform::NORMAL;
    }
    if (str == "90") {
        return Transform::ROTATE_90;
    }
    if (str == "180") {
        return Transform::ROTATE_180;
    }
    if (str == "270") {
        return Transform::ROTATE_270;
    }
    if (str == "flipped") {
        return Transform::FLIPPED;
    }
    if (str == "flipped-90") {
        return Transform::FLIPPED_90;
    }
    if (str == "flipped-180") {
        return Transform::FLIPPED_180;
    }
    if (str == "flipped-270") {
        return Transform::FLIPPED_270;
    }

    throw std::invalid_argument("Unknown transform: " + str);
}

auto transform_utils::to_string(Transform transform) -> std::string {
    std::string result;
    switch (transform) {
        case Transform::NORMAL:
            result = "normal";
            break;
        case Transform::ROTATE_90:
            result = "90";
            break;
        case Transform::ROTATE_180:
            result = "180";
            break;
        case Transform::ROTATE_270:
            result = "270";
            break;
        case Transform::FLIPPED:
            result = "flipped";
            break;
        case Transform::FLIPPED_90:
            result = "flipped-90";
            break;
        case Transform::FLIPPED_180:
            result = "flipped-180";
            break;
        case Transform::FLIPPED_270:
            result = "flipped-270";
            break;
        default:
            throw std::invalid_argument(
                "Unknown transform: " + std::to_string(static_cast<int>(transform))
            );
    }
    return result;
}

auto transform_utils::is_flipped(Transform transform) -> bool {
    return transform == Transform::FLIPPED || transform == Transform::FLIPPED_90 ||
        transform == Transform::FLIPPED_180 || transform == Transform::FLIPPED_270;
}

auto transform_utils::get_flipped(Transform transform) -> Transform {
    Transform result;

    switch (transform) {
        case Transform::NORMAL:
            result = Transform::FLIPPED;
            break;
        case Transform::ROTATE_90:
            result = Transform::FLIPPED_90;
            break;
        case Transform::ROTATE_180:
            result = Transform::FLIPPED_180;
            break;
        case Transform::ROTATE_270:
            result = Transform::FLIPPED_270;
            break;
        case Transform::FLIPPED:
            result = Transform::NORMAL;
            break;
        case Transform::FLIPPED_90:
            result = Transform::ROTATE_90;
            break;
        case Transform::FLIPPED_180:
            result = Transform::ROTATE_180;
            break;
        case Transform::FLIPPED_270:
            result = Transform::ROTATE_270;
            break;
        default:
            throw std::invalid_argument(
                "Unknown transform: " + std::to_string(static_cast<int>(transform))
            );
    }

    return result;
}

auto transform_utils::all_enums() -> const std::vector<Transform> & {
    static const std::vector<Transform> all_enums = {
        Transform::NORMAL,  Transform::ROTATE_90,  Transform::ROTATE_180,  Transform::ROTATE_270,
        Transform::FLIPPED, Transform::FLIPPED_90, Transform::FLIPPED_180, Transform::FLIPPED_270
    };
    return all_enums;
}

auto transform_utils::all_strings() -> const std::vector<std::string> & {
    static const std::vector<std::string> all_strings = [] {
        std::vector<std::string> strings;
        for (const auto & transform : all_enums()) {
            strings.push_back(to_string(transform));
        }
        return strings;
    }();
    return all_strings;
}

auto transform_utils::index_of(Transform transform) -> int {
    const auto & enums = all_enums();
    for (size_t i = 0; i < enums.size(); ++i) {
        if (enums[i] == transform) {
            return static_cast<int>(i);
        }
    }

    throw std::invalid_argument("Transform not found in list");
}

Mode::Mode(int width, int height, float refresh_rate, bool is_preferred, bool is_current)
    : width(width),
      height(height),
      refresh_rate(refresh_rate),
      is_preferred(is_preferred),
      is_current(is_current) {}

auto Mode::operator==(const Mode & other) const -> bool {
    return width == other.width && height == other.height && refresh_rate == other.refresh_rate;
}

auto Mode::operator!=(const Mode & other) const -> bool {
    return !(*this == other);
}

auto Mode::to_string() const -> std::string {
    return std::to_string(width) + "x" + std::to_string(height) + "@" +
        std::to_string(refresh_rate) + "Hz";
}

auto Mode::to_wlr_randr_arg() const -> std::string {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%dx%d@%.6fHz", width, height, refresh_rate);
    return buffer;
}

MonitorSpecs::MonitorSpecs(
    std::string name,
    const std::optional<std::string> & make,
    const std::optional<std::string> & model,
    const std::optional<std::string> & serial_number,
    std::string description,
    const PhysicalSize & physical_size,
    bool enabled,
    const std::vector<Mode> & modes,
    const std::optional<EnabledMonitorSettings> & enabled_monitor_settings
)
    : name(std::move(name)),
      make(make),
      model(model),
      serial_number(serial_number),
      description(std::move(description)),
      physical_size(physical_size),
      enabled(enabled),
      modes(modes),
      enabled_monitor_settings(enabled_monitor_settings) {}

// Getters
auto MonitorSpecs::get_name() const -> const std::string & {
    return name;
}

auto MonitorSpecs::get_description() const -> const std::string & {
    return description;
}

auto MonitorSpecs::is_enabled() const -> bool {
    return enabled;
}

auto MonitorSpecs::get_modes() const -> const std::vector<Mode> & {
    return modes;
}

auto MonitorSpecs::get_enabled_monitor_settings() const -> const std::optional<EnabledMonitorSettings> & {
    return enabled_monitor_settings;
}

auto MonitorSpecs::get_enabled_monitor_settings() -> std::optional<EnabledMonitorSettings> & {
    return enabled_monitor_settings;
}

// Setters
void MonitorSpecs::set_enabled(bool enabled) {
    this->enabled = enabled;

    if (enabled && !enabled_monitor_settings.has_value()) {
        size_t mode_index = 0;
        for (size_t i = 0; i < modes.size(); ++i) {
            if (modes[i].is_preferred) {
                mode_index = i;
                break;
            }
        }

        enabled_monitor_settings = EnabledMonitorSettings(
            mode_index, Position{ 0, 0 }, Transform::NORMAL, 1.0F, false
        );
        spdlog::info("Created default settings for monitor {}", name);
    }
}

void MonitorSpecs::sync_from(const MonitorSpecs & other) {
    if (name != other.name) {
        throw std::invalid_argument("Cannot sync monitors with different names");
    }

    enabled = other.enabled;
    if (other.enabled_monitor_settings.has_value()) {
        enabled_monitor_settings = other.enabled_monitor_settings;
    } else if (other.enabled) {
        enabled_monitor_settings = std::nullopt;
    }
}

EnabledMonitorSettings::EnabledMonitorSettings(
    size_t active_mode_index,
    const Position & position,
    const Transform & transform,
    float scale,
    bool adaptive_sync
)
    : active_mode_index(active_mode_index),
      position(position),
      transform(transform),
      scale(scale),
      adaptive_sync(adaptive_sync) {}

// Getters
auto EnabledMonitorSettings::get_active_mode_index() const -> size_t {
    return active_mode_index;
}

auto EnabledMonitorSettings::get_position() const -> const Position & {
    return position;
}

auto EnabledMonitorSettings::get_transform() const -> const Transform & {
    return transform;
}

auto EnabledMonitorSettings::get_scale() const -> float {
    return scale;
}

auto EnabledMonitorSettings::is_adaptive_sync() const -> bool {
    return adaptive_sync;
}

// Setters
void EnabledMonitorSettings::set_active_mode_index(size_t index) {
    active_mode_index = index;
}

void EnabledMonitorSettings::set_position(int x, int y) {
    position.x = x;
    position.y = y;
}

void EnabledMonitorSettings::set_position_x(int x) {
    position.x = x;
}

void EnabledMonitorSettings::set_position_y(int y) {
    position.y = y;
}

void EnabledMonitorSettings::set_transform(const Transform & t) {
    transform = t;
}

void EnabledMonitorSettings::set_scale(float scale) {
    this->scale = scale;
}

void EnabledMonitorSettings::set_adaptive_sync(bool adaptive_sync) {
    this->adaptive_sync = adaptive_sync;
}

namespace {

auto resolve_active_mode_index(const std::vector<Mode> & modes, int active_mode_index) -> size_t {
    if (active_mode_index >= 0 && static_cast<size_t>(active_mode_index) < modes.size()) {
        return static_cast<size_t>(active_mode_index);
    }

    for (size_t i = 0; i < modes.size(); ++i) {
        if (modes[i].is_preferred) {
            spdlog::warn("No current mode found, using preferred mode index {}", i);
            return i;
        }
    }

    spdlog::warn("No current or preferred mode found, using index 0");
    return 0;
}

}  // namespace

auto get_monitor_specs_list() -> std::vector<MonitorSpecs> {
    std::vector<MonitorSpecs> monitor_specs_list;

    std::string output;
    try {
        output = run_command("wlr-randr", { "--json" });
    } catch (const std::exception & e) {
        spdlog::error("Failed to run wlr-randr: {}", e.what());
        return monitor_specs_list;
    }

    if (output.empty()) {
        spdlog::error("wlr-randr returned empty output");
        return monitor_specs_list;
    }

    json j;
    try {
        j = json::parse(output);
    } catch (const json::exception & e) {
        spdlog::error("Failed to parse wlr-randr JSON: {}", e.what());
        return monitor_specs_list;
    }

    spdlog::info("Found {} monitors", j.size());

    for (auto & monitor : j) {
        try {
            spdlog::debug("Processing monitor: {}", monitor["name"].get<std::string>());
            bool const is_enabled = monitor["enabled"].get<bool>();
            spdlog::debug("Monitor is enabled: {}", is_enabled);

            std::vector<Mode> modes;
            int active_mode_index = -1;
            for (auto & mode : monitor["modes"]) {
                modes.emplace_back(Mode(
                    mode["width"].get<int>(),
                    mode["height"].get<int>(),
                    mode["refresh"].get<float>(),
                    mode["preferred"].get<bool>(),
                    mode["current"].get<bool>()
                ));

                if (mode["current"]) {
                    active_mode_index = static_cast<int>(modes.size()) - 1;
                }
            }

            std::optional<EnabledMonitorSettings> enabled_monitor_settings = std::nullopt;
            if (is_enabled) {
                const size_t resolved_mode_index =
                    resolve_active_mode_index(modes, active_mode_index);
                enabled_monitor_settings = EnabledMonitorSettings(
                    resolved_mode_index,
                    Position{ monitor["position"]["x"].get<int>(),
                              monitor["position"]["y"].get<int>() },
                    transform_utils::from_string(monitor["transform"].get<std::string>()),
                    monitor["scale"].get<float>(),
                    monitor["adaptive_sync"].get<bool>()
                );
            }

            std::optional<std::string> make = std::nullopt;
            std::optional<std::string> model = std::nullopt;
            std::optional<std::string> serial_number = std::nullopt;
            if (monitor.contains("make") && !monitor["make"].is_null()) {
                make = monitor["make"];
            }
            if (monitor.contains("model") && !monitor["model"].is_null()) {
                model = monitor["model"];
            }
            if (monitor.contains("serial") && !monitor["serial"].is_null()) {
                serial_number = monitor["serial"];
            }

            const PhysicalSize physical_size = { monitor["physical_size"]["width"].get<int>(),
                                                 monitor["physical_size"]["height"].get<int>() };

            monitor_specs_list.emplace_back(MonitorSpecs(
                monitor["name"],
                make,
                model,
                serial_number,
                monitor["description"],
                physical_size,
                is_enabled,
                modes,
                enabled_monitor_settings
            ));
        } catch (const std::exception & e) {
            const std::string name =
                monitor.contains("name") ? monitor["name"].get<std::string>() : "unknown";
            spdlog::warn("Skipping monitor {} due to parse error: {}", name, e.what());
        }
    }

    return monitor_specs_list;
}
