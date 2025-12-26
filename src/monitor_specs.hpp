#pragma once
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

struct PhysicalSize {
    int width_mm;
    int height_mm;
};

struct Mode {
    const int width;
    const int height;
    const float refresh_rate;
    const bool is_preferred;
    bool is_current;

    Mode(int width, int height, float refresh_rate, bool is_preferred, bool is_current);

    auto operator==(const Mode & other) const -> bool;
    auto operator!=(const Mode & other) const -> bool;

    [[nodiscard]] auto to_string() const -> std::string;
    [[nodiscard]] auto to_wlr_randr_arg() const -> std::string;
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

namespace transform_utils {
auto from_string(const std::string & str) -> Transform;
auto to_string(Transform transform) -> std::string;
auto is_flipped(Transform transform) -> bool;
auto get_flipped(Transform transform) -> Transform;
auto all_enums() -> const std::vector<Transform> &;
auto all_strings() -> const std::vector<std::string> &;
auto index_of(Transform transform) -> int;
}  // namespace transform_utils

class EnabledMonitorSettings {
public:
    EnabledMonitorSettings(
        size_t active_mode_index,
        const Position & position,
        const Transform & transform,
        float scale,
        bool adaptive_sync
    );

    EnabledMonitorSettings() = default;

    // Getters
    [[nodiscard]] auto get_active_mode_index() const -> size_t;
    [[nodiscard]] auto get_position() const -> const Position &;
    [[nodiscard]] auto get_transform() const -> const Transform &;
    [[nodiscard]] auto get_scale() const -> float;
    [[nodiscard]] auto is_adaptive_sync() const -> bool;

    // Setters
    void set_active_mode_index(size_t index);
    void set_position(int x, int y);
    void set_position_x(int x);
    void set_position_y(int y);
    void set_transform(const Transform & transform);
    void set_scale(float scale);
    void set_adaptive_sync(bool adaptive_sync);

private:
    size_t active_mode_index = 0;
    Position position = { 0, 0 };
    Transform transform = Transform::NORMAL;
    float scale = 1.0F;
    bool adaptive_sync = false;
};

class MonitorSpecs {
public:
    MonitorSpecs(
        std::string name,
        const std::optional<std::string> & make,
        const std::optional<std::string> & model,
        const std::optional<std::string> & serial_number,
        std::string description,
        const PhysicalSize & physical_size,
        bool enabled,
        const std::vector<Mode> & modes,
        const std::optional<EnabledMonitorSettings> & enabled_monitor_settings = std::nullopt
    );

    // Getters
    [[nodiscard]] auto get_name() const -> const std::string &;
    [[nodiscard]] auto get_description() const -> const std::string &;
    [[nodiscard]] auto is_enabled() const -> bool;
    [[nodiscard]] auto get_modes() const -> const std::vector<Mode> &;
    [[nodiscard]] auto get_enabled_monitor_settings() const
        -> const std::optional<EnabledMonitorSettings> &;
    auto get_enabled_monitor_settings() -> std::optional<EnabledMonitorSettings> &;

    // Setters
    void set_enabled(bool enabled);
    void sync_from(const MonitorSpecs & other);

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
};

auto get_monitor_specs_list() -> std::vector<MonitorSpecs>;
