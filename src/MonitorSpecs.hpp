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
    Mode *active_mode;
    const std::vector<Mode> modes;
    std::optional<Position> position;
    std::optional<Transform> transform;
    std::optional<float> scale;
    std::optional<bool> adaptive_sync;

   public:
    MonitorSpecs(const std::string &name, const std::optional<std::string> &make, const std::optional<std::string> &model,
                 const std::optional<std::string> &serial_number, const std::string &description, const PhysicalSize &physical_size, bool enabled,
                 Mode *active_mode, const std::vector<Mode> &modes, const std::optional<Position> &position,
                 const std::optional<Transform> &transform, const std::optional<float> &scale, const std::optional<bool> &adaptive_sync);

    // Getters
    const std::string &getName() const;
    const std::string &getDescription() const;
    bool isEnabled() const;
    const Mode *getActiveMode() const;
    const std::vector<Mode> &getModes() const;
    const std::optional<Position> &getPosition() const;
    const std::optional<Transform> &getTransform() const;
    const std::optional<float> &getScale() const;
    const std::optional<bool> &isAdaptiveSync() const;

    // Setters
    void setEnabled(bool enabled);
    void setActiveMode(Mode *active_mode);
    void setPosition(int x, int y);
    void setTransform(const Transform &transform);
    void setScale(float scale);
    void setAdaptiveSync(bool adaptive_sync);
};

std::vector<MonitorSpecs> getMonitorSpecsList();
