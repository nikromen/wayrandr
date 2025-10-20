#include "MonitorSpecs.hpp"
#include "utils/Helpers.hpp"
#include <nlohmann/json.hpp>
#include <optional>
#include <unordered_map>
#include <stdexcept>
#include <QString>
#include <spdlog/spdlog.h>

using json = nlohmann::json;

Transform TransformUtils::fromString(const std::string &str) {
    static const std::unordered_map<std::string, Transform> mapping = {
        {"normal", Transform::NORMAL},
        {"90", Transform::ROTATE_90},
        {"180", Transform::ROTATE_180},
        {"270", Transform::ROTATE_270},
        {"flipped", Transform::FLIPPED},
        {"flipped-90", Transform::FLIPPED_90},
        {"flipped-180", Transform::FLIPPED_180},
        {"flipped-270", Transform::FLIPPED_270}
    };
    
    auto it = mapping.find(str);
    if (it != mapping.end()) {
        spdlog::debug("Transform mapping found: {} to {}", str, static_cast<int>(it->second));
        return it->second;
    }
    
    spdlog::warn("Unknown transform: {}, defaulting to NORMAL", str);
    return Transform::NORMAL;
}

std::string TransformUtils::toString(Transform transform) {
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
            throw std::invalid_argument("Unknown transform: " + std::to_string(static_cast<int>(transform)));
    }
    spdlog::debug("Transform converted to string: {} to {}", static_cast<int>(transform), result);
    return result;
}

bool TransformUtils::isFlipped(Transform transform) {
    return transform == Transform::FLIPPED || transform == Transform::FLIPPED_90 ||
           transform == Transform::FLIPPED_180 || transform == Transform::FLIPPED_270;
}

Transform TransformUtils::getFlipped(Transform transform) {
    Transform result;
    spdlog::debug("Getting flipped transform for: {}", static_cast<int>(transform));
    
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
            throw std::invalid_argument("Unknown transform: " + std::to_string(static_cast<int>(transform)));
    }
    
    spdlog::debug("Flipped transform result: {} -> {}", static_cast<int>(transform), static_cast<int>(result));
    return result;
}

const std::vector<Transform> &TransformUtils::allEnums() {
    static const std::vector<Transform> all_enums = {
        Transform::NORMAL,
        Transform::ROTATE_90,
        Transform::ROTATE_180,
        Transform::ROTATE_270,
        Transform::FLIPPED,
        Transform::FLIPPED_90,
        Transform::FLIPPED_180,
        Transform::FLIPPED_270
    };
    return all_enums;
}

const std::vector<std::string> &TransformUtils::allStrings() {
    static const std::vector<std::string> all_strings = [] {
        std::vector<std::string> strings;
        for (const auto &transform : allEnums()) {
            strings.push_back(toString(transform));
        }
        return strings;
    }();
    return all_strings;
}

int TransformUtils::indexOf(Transform transform) {
    const auto &enums = allEnums();
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

bool Mode::operator==(const Mode &other) const {
    return width == other.width && height == other.height && refresh_rate == other.refresh_rate;
}

bool Mode::operator!=(const Mode &other) const { return !(*this == other); }

std::string Mode::toString() const {
    return std::to_string(width) + "x" + std::to_string(height) + "@" +
           std::to_string(refresh_rate) + "Hz";
}

MonitorSpecs::MonitorSpecs(const std::string &name, const std::optional<std::string> &make,
                           const std::optional<std::string> &model, const std::optional<std::string> &serial_number, const std::string &description,
                           const PhysicalSize &physical_size, bool enabled,
                           const std::vector<Mode> &modes, const std::optional<EnabledMonitorSettings> &enabled_monitor_settings)
    : name(name),
      make(make),
      model(model),
      serial_number(serial_number),
      description(description),
      physical_size(physical_size),
      enabled(enabled),
      modes(modes),
      enabled_monitor_settings(enabled_monitor_settings) {}

// Getters
const std::string &MonitorSpecs::getName() const { return name; }
const std::string &MonitorSpecs::getDescription() const { return description; }
bool MonitorSpecs::isEnabled() const { return enabled; }
const std::vector<Mode> &MonitorSpecs::getModes() const { return modes; }
const std::optional<EnabledMonitorSettings> &MonitorSpecs::getEnabledMonitorSettings() const { return enabled_monitor_settings; }
std::optional<EnabledMonitorSettings> &MonitorSpecs::getEnabledMonitorSettings() { return enabled_monitor_settings; }

// Setters
void MonitorSpecs::setEnabled(bool enabled) {
    this->enabled = enabled;
    // No need for deleting settings when disabling; user may re-enable it, so remember them
}

EnabledMonitorSettings::EnabledMonitorSettings(size_t active_mode_index, 
                                                     const Position &position,
                                                     const Transform &transform,
                                                     float scale,
                                                     bool adaptive_sync)
    : active_mode_index(active_mode_index),
      position(position),
      transform(transform),
      scale(scale),
      adaptive_sync(adaptive_sync) {}

// Getters
size_t EnabledMonitorSettings::getActiveModeIndex() const { return active_mode_index; }
const Position& EnabledMonitorSettings::getPosition() const { return position; }
const Transform& EnabledMonitorSettings::getTransform() const { return transform; }
float EnabledMonitorSettings::getScale() const { return scale; }
bool EnabledMonitorSettings::isAdaptiveSync() const { return adaptive_sync; }

// Setters
void EnabledMonitorSettings::setActiveModeIndex(size_t index) {
    // Note: Ideally, we should validate that index is within a valid range
    // but since we don't have direct access to the modes here,
    // we'll simply set the index and assume the caller validates it
    active_mode_index = index;
}

void EnabledMonitorSettings::setPosition(int x, int y) {
    position.x = x;
    position.y = y;
}

void EnabledMonitorSettings::setPositionX(int x) {
    position.x = x;
}

void EnabledMonitorSettings::setPositionY(int y) {
    position.y = y;
}

void EnabledMonitorSettings::setTransform(const Transform &t) {
    transform = t;
}

void EnabledMonitorSettings::setScale(float scale) {
    this->scale = scale;
}

void EnabledMonitorSettings::setAdaptiveSync(bool adaptive_sync) {
    this->adaptive_sync = adaptive_sync;
}

std::vector<MonitorSpecs> getMonitorSpecsList() {
    std::vector<MonitorSpecs> monitor_specs_list;

    std::string output = runCommand("wlr-randr --json");
    json j = json::parse(output);
    spdlog::info("Found {} monitors", j.size());

    for (auto &monitor : j) {
        spdlog::debug("Processing monitor: {}", monitor["name"].get<std::string>());
        bool is_enabled = monitor["enabled"].get<bool>();
        spdlog::debug("Monitor is enabled: {}", is_enabled);

        std::vector<Mode> modes;
        int active_mode_index = -1;
        for (auto &mode : monitor["modes"]) {
            modes.emplace_back(
                mode["width"].get<int>(),
                mode["height"].get<int>(),
                mode["refresh"].get<float>(),
                mode["preferred"].get<bool>(),
                mode["current"].get<bool>()
            );

            if (mode["current"]) {
                active_mode_index = modes.size() - 1;
            }
        }

        std::optional<EnabledMonitorSettings> enabled_monitor_settings = std::nullopt;
        if (is_enabled) {
            enabled_monitor_settings = EnabledMonitorSettings(
                static_cast<size_t>(active_mode_index),
                Position{monitor["position"]["x"].get<int>(), monitor["position"]["y"].get<int>()},
                TransformUtils::fromString(monitor["transform"].get<std::string>()),
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

        const PhysicalSize physical_size = {monitor["physical_size"]["width"].get<int>(), monitor["physical_size"]["height"].get<int>()};

        monitor_specs_list.emplace_back(
            MonitorSpecs(
                monitor["name"],
                make,
                model,
                serial_number,
                monitor["description"],
                physical_size,
                is_enabled,
                modes,
                enabled_monitor_settings
            )
        );
    }
    return monitor_specs_list;
}
