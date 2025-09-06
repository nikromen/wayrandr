#include "MonitorSpecs.hpp"
#include "utils/Helpers.hpp"
#include <nlohmann/json.hpp>
#include <optional>
#include <unordered_map>

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
        return it->second;
    }
    
    // default
    return Transform::NORMAL;
}

std::string TransformUtils::toString(Transform transform) {
    switch (transform) {
        case Transform::NORMAL:
            return "normal";
        case Transform::ROTATE_90:
            return "90";
        case Transform::ROTATE_180:
            return "180";
        case Transform::ROTATE_270:
            return "270";
        case Transform::FLIPPED:
            return "flipped";
        case Transform::FLIPPED_90:
            return "flipped-90";
        case Transform::FLIPPED_180:
            return "flipped-180";
        case Transform::FLIPPED_270:
            return "flipped-270";
        default:
            return "normal";
    }
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
                           const PhysicalSize &physical_size, bool enabled, Mode *active_mode,
                           const std::vector<Mode> &modes, const std::optional<Position> &position,
                           const std::optional<Transform> &transform, const std::optional<float> &scale, const std::optional<bool> &adaptive_sync)
    : name(name),
      make(make),
      model(model),
      description(description),
      serial_number(serial_number),
      physical_size(physical_size),
      enabled(enabled),
      active_mode(active_mode),
      modes(modes),
      position(position),
      transform(transform),
      scale(scale),
      adaptive_sync(adaptive_sync) {}

// Getters
const std::string &MonitorSpecs::getName() const { return name; }
const std::string &MonitorSpecs::getDescription() const { return description; }
bool MonitorSpecs::isEnabled() const { return enabled; }
const Mode *MonitorSpecs::getActiveMode() const { return active_mode; }
const std::vector<Mode> &MonitorSpecs::getModes() const { return modes; }
const std::optional<Position> &MonitorSpecs::getPosition() const { return position; }
const std::optional<Transform> &MonitorSpecs::getTransform() const { return transform; }
const std::optional<float> &MonitorSpecs::getScale() const { return scale; }
const std::optional<bool> &MonitorSpecs::isAdaptiveSync() const { return adaptive_sync; }

// Setters
void MonitorSpecs::setEnabled(bool enabled) { this->enabled = enabled; }
void MonitorSpecs::setTransform(const Transform &transform) { this->transform = transform; }
void MonitorSpecs::setScale(float scale) { this->scale = scale; }
void MonitorSpecs::setAdaptiveSync(bool adaptive_sync) { this->adaptive_sync = adaptive_sync; }

void MonitorSpecs::setActiveMode(Mode *active_mode) {
    this->active_mode->is_current = false;
    this->active_mode = active_mode;
    this->active_mode->is_current = true;
}

void MonitorSpecs::setPosition(int x, int y) {
    if (!this->position) {
        this->position = Position{x, y};
    } else {
        this->position->x = x;
        this->position->y = y;
    }
}

std::vector<MonitorSpecs> getMonitorSpecsList() {
    std::vector<MonitorSpecs> monitor_specs_list;

    std::string output = runCommand("wlr-randr --json");
    json j = json::parse(output);
    for (auto &monitor : j) {
        std::vector<Mode> modes;
        Mode *active_mode = nullptr;
        for (auto &mode : monitor["modes"]) {
            modes.emplace_back(
                mode["width"].get<int>(),
                mode["height"].get<int>(),
                mode["refresh"].get<float>(),
                mode["preferred"].get<bool>(),
                mode["current"].get<bool>()
            );

            if (mode["current"]) {
                active_mode = &modes.back();
            }
        }

        bool is_enabled = monitor["enabled"].get<bool>();
        std::optional<Position> position = std::nullopt;
        std::optional<Transform> transform = std::nullopt;
        std::optional<float> scale = std::nullopt;
        std::optional<bool> adaptive_sync = std::nullopt;
        if (is_enabled) {
            position = Position{monitor["position"]["x"].get<int>(), monitor["position"]["y"].get<int>()};
            transform = TransformUtils::fromString(monitor["transform"].get<std::string>());
            scale = monitor["scale"].get<float>();
            adaptive_sync = monitor["adaptive_sync"].get<bool>();
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
                active_mode,
                modes,
                position,
                transform,
                scale,
                adaptive_sync
            )
        );
    }
    return monitor_specs_list;
}
