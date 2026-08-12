#include "models/monitor_properties.hpp"

#include <qcontainerfwd.h>
#include <qtmetamacros.h>
#include <spdlog/spdlog.h>

#include <QMetaObject>
#include <QObject>
#include <QPoint>
#include <QString>
#include <QStringList>
#include <cmath>
#include <cstddef>
#include <memory>
#include <utility>

#include "models/main_window.hpp"
#include "monitor_specs.hpp"
#include "utils/canvas_drag.hpp"
#include "utils/canvas_layout.hpp"
#include "utils/transform_list.hpp"

MonitorProperties::MonitorProperties(MonitorSpecs specs, QObject * parent)
    : QObject(parent),
      monitor_specs(std::make_unique<MonitorSpecs>(std::move(specs))) {
    spdlog::debug("Created MonitorProperties for monitor: {}", monitor_specs->get_name());
}

void MonitorProperties::set_dirty(bool dirty) {
    if (dirty_ == dirty) {
        return;
    }
    dirty_ = dirty;
    emit dirty_changed();
}

void MonitorProperties::set_connected(bool connected) {
    if (connected_ == connected) {
        return;
    }
    connected_ = connected;
    emit connected_changed();
    notify_all_changed();
}

void MonitorProperties::refresh_from(const MonitorSpecs & specs, bool preserve_draft) {
    if (preserve_draft && dirty_) {
        return;
    }
    auto refreshed = std::make_unique<MonitorSpecs>(specs);
    // Keep the existing cache of settings when a connected output is disabled,
    // including mode remapping; replace metadata as well as mutable live values.
    monitor_specs->sync_from(specs);
    refreshed->get_enabled_monitor_settings() = monitor_specs->get_enabled_monitor_settings();
    monitor_specs = std::move(refreshed);
    set_dirty(false);
    notify_all_changed();
}

// Getters
auto MonitorProperties::is_enabled() const -> bool {
    return connected_ && monitor_specs->is_enabled();
}

auto MonitorProperties::has_settings() const -> bool {
    return is_enabled() && monitor_specs->get_enabled_monitor_settings().has_value();
}

auto MonitorProperties::is_adaptive_sync() const -> bool {
    if (!has_settings()) {
        return false;
    }
    return monitor_specs->get_enabled_monitor_settings().value().is_adaptive_sync();
}

auto MonitorProperties::get_name() const -> QString {
    return QString::fromStdString(monitor_specs->get_name());
}

auto MonitorProperties::get_description() const -> QString {
    return QString::fromStdString(monitor_specs->get_description());
}

auto MonitorProperties::get_scale() const -> float {
    if (!has_settings()) {
        return 1.0F;
    }
    return monitor_specs->get_enabled_monitor_settings().value().get_scale();
}

auto MonitorProperties::get_position_x() const -> int {
    if (!has_settings()) {
        return 0;
    }
    return monitor_specs->get_enabled_monitor_settings().value().get_position().x;
}

auto MonitorProperties::get_position_y() const -> int {
    if (!has_settings()) {
        return 0;
    }
    return monitor_specs->get_enabled_monitor_settings().value().get_position().y;
}

auto MonitorProperties::get_resolution(bool width) const -> int {
    if (!has_settings()) {
        return 0;
    }
    auto index = monitor_specs->get_enabled_monitor_settings().value().get_active_mode_index();
    const auto & modes = monitor_specs->get_modes();
    if (index < modes.size()) {
        if (width) {
            return modes[index].width;
        }
        return modes[index].height;
    }

    // QML also reads dimensions when there is no available resolution.
    return 0;
}

auto MonitorProperties::get_resolution_width() const -> int {
    return get_resolution(true);
}

auto MonitorProperties::get_resolution_height() const -> int {
    return get_resolution(false);
}

auto MonitorProperties::get_resolutions() const -> QStringList {
    QStringList resolutions;
    for (const auto & mode : monitor_specs->get_modes()) {
        resolutions.append(QString::fromStdString(mode.to_string()));
    }
    return resolutions;
}

auto MonitorProperties::get_transform_list() -> QStringList {
    return transform_list::as_qstring_list();
}

auto MonitorProperties::get_transform() const -> QString {
    if (!has_settings()) {
        return QString::fromStdString(transform_utils::to_string(Transform::NORMAL));
    }
    return QString::fromStdString(
        transform_utils::to_string(
            monitor_specs->get_enabled_monitor_settings().value().get_transform()
        )
    );
}

auto MonitorProperties::get_active_resolution_index() const -> size_t {
    if (!has_settings()) {
        return 0;
    }
    return monitor_specs->get_enabled_monitor_settings().value().get_active_mode_index();
}

auto MonitorProperties::is_transform_rotated() const -> bool {
    if (!has_settings()) {
        return false;
    }

    return transform_utils::is_rotated(get_transform().toStdString());
}

auto MonitorProperties::get_layout_width() const -> int {
    if (!has_settings()) {
        return 0;
    }

    return canvas_layout::compute_layout_size(
               get_resolution_width(), get_resolution_height(), get_scale(), is_transform_rotated()
    )
        .width;
}

auto MonitorProperties::get_layout_height() const -> int {
    if (!has_settings()) {
        return 0;
    }

    return canvas_layout::compute_layout_size(
               get_resolution_width(), get_resolution_height(), get_scale(), is_transform_rotated()
    )
        .height;
}

// Setters
auto MonitorProperties::editing_allowed() const -> bool {
    // Window-owned models share the GUI admission gate; standalone models stay editable.
    const auto * window = qobject_cast<MainWindow *>(parent());
    return connected_ && (window == nullptr || window->is_direct_editing_allowed());
}

void MonitorProperties::set_enabled(bool enabled) {
    if (!editing_allowed() || is_enabled() == enabled) {
        return;
    }

    monitor_specs->set_enabled(enabled);
    set_dirty(true);
    emit enabled_changed();
    emit layout_dimensions_changed();
}

void MonitorProperties::set_adaptive_sync(bool adaptive_sync) {
    if (!editing_allowed() || !has_settings()) {
        return;
    }

    if (is_adaptive_sync() == adaptive_sync) {
        return;
    }

    auto & settings = monitor_specs->get_enabled_monitor_settings().value();
    settings.set_adaptive_sync(adaptive_sync);
    set_dirty(true);
    emit adaptive_sync_changed();
}

void MonitorProperties::set_scale(float scale) {
    if (!editing_allowed() || !has_settings()) {
        return;
    }

    if (std::abs(get_scale() - scale) < 0.001F) {
        return;
    }

    auto & settings = monitor_specs->get_enabled_monitor_settings().value();
    settings.set_scale(scale);
    set_dirty(true);
    emit scale_changed();
    emit layout_dimensions_changed();
}

void MonitorProperties::set_position_x(int x) {
    if (!editing_allowed() || !has_settings()) {
        return;
    }

    if (get_position_x() == x) {
        return;
    }

    auto & settings = monitor_specs->get_enabled_monitor_settings().value();
    settings.set_position_x(x);
    set_dirty(true);
    emit position_x_changed();
}

void MonitorProperties::set_position_y(int y) {
    if (!editing_allowed() || !has_settings()) {
        return;
    }

    if (get_position_y() == y) {
        return;
    }

    auto & settings = monitor_specs->get_enabled_monitor_settings().value();
    settings.set_position_y(y);
    set_dirty(true);
    emit position_y_changed();
}

void MonitorProperties::set_active_resolution_index(size_t index) {
    if (!editing_allowed() || !has_settings() || index >= monitor_specs->get_modes().size()) {
        return;
    }

    if (get_active_resolution_index() == index) {
        return;
    }

    auto & settings = monitor_specs->get_enabled_monitor_settings().value();
    settings.set_active_mode_index(index);
    set_dirty(true);
    emit active_resolution_index_changed();
    emit layout_dimensions_changed();
}

void MonitorProperties::set_transform(const QString & transform) {
    if (!editing_allowed() || !has_settings()) {
        return;
    }

    auto & settings = monitor_specs->get_enabled_monitor_settings().value();
    Transform const current_transform = settings.get_transform();
    Transform new_transform = transform_utils::from_string(transform.toStdString());

    if (current_transform == new_transform) {
        spdlog::debug("Transform not changed for monitor {}", monitor_specs->get_name());
        return;
    }

    spdlog::debug(
        "Changing transform for monitor {} from {} to {}",
        monitor_specs->get_name(),
        static_cast<int>(current_transform),
        static_cast<int>(new_transform)
    );
    settings.set_transform(new_transform);
    set_dirty(true);
    emit transform_changed();
    emit layout_dimensions_changed();
}

void MonitorProperties::start_drag(int mouse_x, int mouse_y) {
    if (!editing_allowed()) {
        return;
    }
    canvas_drag::start(drag_state_, mouse_x, mouse_y, get_position_x(), get_position_y());
}

void MonitorProperties::update_drag(
    int mouse_x,
    int mouse_y,
    float display_scale,
    QObject * main_window,
    int canvas_width,
    int canvas_height
) {
    if (!editing_allowed()) {
        return;
    }
    canvas_drag::update_with_snap(
        drag_state_,
        mouse_x,
        mouse_y,
        display_scale,
        this,
        main_window,
        canvas_width,
        canvas_height,
        canvas_layout::K_DEFAULT_SNAP_THRESHOLD
    );
}

void MonitorProperties::activate_preferred_mode() {
    if (!editing_allowed()) {
        return;
    }
    if (!has_settings()) {
        spdlog::warn(
            "Cannot activate preferred mode: settings not available for monitor {}",
            monitor_specs->get_name()
        );
        return;
    }

    const auto & modes = monitor_specs->get_modes();
    for (size_t i = 0; i < modes.size(); ++i) {
        if (modes[i].is_preferred) {
            spdlog::info(
                "Activating preferred mode for monitor {}: {}",
                monitor_specs->get_name(),
                modes[i].to_string()
            );
            set_active_resolution_index(i);
            return;
        }
    }

    spdlog::warn("No preferred mode found for monitor {}", monitor_specs->get_name());
}

void MonitorProperties::notify_all_changed() {
    emit description_changed();
    emit resolutions_changed();
    emit enabled_changed();
    emit adaptive_sync_changed();
    emit active_resolution_index_changed();
    emit scale_changed();
    emit position_x_changed();
    emit position_y_changed();
    emit transform_changed();
    emit layout_dimensions_changed();
}
