#pragma once
#include <string>
#include <vector>
#include <optional>

struct PhysicalSize {
    int width_mm;
    int height_mm;
};

struct Mode {
    const int width;
    const int height;
    const float refresh_rate;
    bool is_preferred;
    bool is_current;

    Mode(int width, int height, float refresh_rate, bool is_preferred, bool is_current);

    bool operator==(const Mode &other) const;
    bool operator!=(const Mode &other) const;

    std::string toString() const;
};

struct Position {
    int x;
    int y;
};

enum class Transform {
    NORMAL,
    ROTATE_90,
    ROTATE_180,
    ROTATE_270,
    FLIPPED,
    FLIPPED_90,
    FLIPPED_180,
    FLIPPED_270
};

namespace TransformUtils {
    Transform fromString(const std::string &str);
    std::string toString(Transform transform);
    bool isFlipped(Transform transform);
    Transform getFlipped(Transform transform);
    const std::vector<Transform> &allEnums();
    const std::vector<std::string> &allStrings();
    int indexOf(Transform transform);
}

class MonitorSpecs {
   private:
    const std::string name;
    const std::optional<std::string> make;
    const std::optional<std::string> model;
    const std::optional<std::string> serial_number;
    const std::string description;
    const PhysicalSize physical_size;
    bool enabled;
    const std::vector<Mode> modes;
    std::optional<EnabledMonitorSettings> enabled_monitor_settings;

   public:
    MonitorSpecs(const std::string &name, const std::optional<std::string> &make, const std::optional<std::string> &model,
                 const std::optional<std::string> &serial_number, const std::string &description, const PhysicalSize &physical_size, bool enabled,
                 const std::vector<Mode> &modes, const std::optional<EnabledMonitorSettings> &enabled_monitor_settings = std::nullopt);

    // Getters
    const std::string &getName() const;
    const std::string &getDescription() const;
    bool isEnabled() const;
    const std::vector<Mode> &getModes() const;
    const std::optional<EnabledMonitorSettings> &getEnabledMonitorSettings() const;

    // Setters
    void setEnabled(bool enabled);
};

class EnabledMonitorSettings {
public:
    EnabledMonitorSettings(size_t active_mode_index, 
                         const Position &position,
                         const Transform &transform,
                         float scale,
                         bool adaptive_sync);

    EnabledMonitorSettings() = default;

    // Getters
    size_t getActiveModeIndex() const;
    const Position& getPosition() const;
    const Transform& getTransform() const;
    float getScale() const;
    bool isAdaptiveSync() const;

    // Setters
    void setActiveModeIndex(size_t index);
    void setPosition(int x, int y);
    void setPositionX(int x);
    void setPositionY(int y);
    void setTransform(const Transform &transform);
    void setScale(float scale);
    void setAdaptiveSync(bool adaptive_sync);

private:
    size_t active_mode_index = 0;
    Position position = {0, 0};
    Transform transform = Transform::NORMAL;
    float scale = 1.0f;
    bool adaptive_sync = false;
};

std::vector<MonitorSpecs> getMonitorSpecsList();
