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
#include <stdexcept>

#include "monitor_specs.hpp"

MonitorProperties::MonitorProperties(MonitorSpecs * monitor_specs, QObject * parent)
    : QObject(parent),
      monitor_specs(monitor_specs) {
    spdlog::debug("Created MonitorProperties for monitor: {}", monitor_specs->get_name());
}

// Getters
auto MonitorProperties::is_enabled() const -> bool {
    return monitor_specs->is_enabled();
}

auto MonitorProperties::has_settings() const -> bool {
    return is_enabled() && monitor_specs->get_enabled_monitor_settings().has_value();
}

auto MonitorProperties::is_adaptive_sync() const -> bool {
    if (!has_settings()) {
        spdlog::debug("Settings not available for monitor {}", monitor_specs->get_name());
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
        spdlog::debug("Settings not available for monitor {}", monitor_specs->get_name());
        return 1.0F;
    }
    return monitor_specs->get_enabled_monitor_settings().value().get_scale();
}

auto MonitorProperties::get_position_x() const -> int {
    if (!has_settings()) {
        spdlog::debug("Settings not available for monitor {}", monitor_specs->get_name());
        return 0;
    }
    return monitor_specs->get_enabled_monitor_settings().value().get_position().x;
}

auto MonitorProperties::get_position_y() const -> int {
    if (!has_settings()) {
        spdlog::debug("Settings not available for monitor {}", monitor_specs->get_name());
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

    throw std::out_of_range("Active mode index is out of range of available modes");
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
    QStringList transform_list;
    for (const auto & transform : transform_utils::all_strings()) {
        transform_list.append(QString::fromStdString(transform));
    }
    return transform_list;
}

auto MonitorProperties::get_transform() const -> QString {
    if (!has_settings()) {
        spdlog::debug("Settings not available for monitor {}", monitor_specs->get_name());
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
        spdlog::debug("Settings not available for monitor {}", monitor_specs->get_name());
        return 0;
    }
    return monitor_specs->get_enabled_monitor_settings().value().get_active_mode_index();
}

// Setters
void MonitorProperties::set_enabled(bool enabled) {
    if (is_enabled() == enabled) {
        return;
    }

    monitor_specs->set_enabled(enabled);
    emit enabled_changed();
}

void MonitorProperties::set_adaptive_sync(bool adaptive_sync) {
    if (is_adaptive_sync() == adaptive_sync) {
        return;
    }

    auto & settings = monitor_specs->get_enabled_monitor_settings().value();
    settings.set_adaptive_sync(adaptive_sync);
    emit adaptive_sync_changed();
}

void MonitorProperties::set_scale(float scale) {
    if (std::abs(get_scale() - scale) < 0.001F) {
        return;
    }

    auto & settings = monitor_specs->get_enabled_monitor_settings().value();
    settings.set_scale(scale);
    emit scale_changed();
}

void MonitorProperties::set_position_x(int x) {
    if (get_position_x() == x) {
        return;
    }

    auto & settings = monitor_specs->get_enabled_monitor_settings().value();
    settings.set_position_x(x);
    emit position_x_changed();
}

void MonitorProperties::set_position_y(int y) {
    if (get_position_y() == y) {
        return;
    }

    auto & settings = monitor_specs->get_enabled_monitor_settings().value();
    settings.set_position_y(y);
    emit position_y_changed();
}

void MonitorProperties::set_active_resolution_index(size_t index) {
    if (get_active_resolution_index() == index) {
        return;
    }

    auto & settings = monitor_specs->get_enabled_monitor_settings().value();
    settings.set_active_mode_index(index);
    emit active_resolution_index_changed();
}

void MonitorProperties::set_transform(const QString & transform) {
    if (!has_settings()) {
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
    emit transform_changed();
}

void MonitorProperties::start_drag(int mouse_x, int mouse_y) {
    drag_start_mouse_x_ = mouse_x;
    drag_start_mouse_y_ = mouse_y;
    drag_start_pos_x_ = get_position_x();
    drag_start_pos_y_ = get_position_y();
}

void MonitorProperties::update_drag(
    int mouse_x, int mouse_y, float display_scale, QObject * main_window
) {
    float const delta_x = (mouse_x - drag_start_mouse_x_) / display_scale;
    float const delta_y = (mouse_y - drag_start_mouse_y_) / display_scale;

    int new_x = drag_start_pos_x_ + static_cast<int>(delta_x);
    int new_y = drag_start_pos_y_ + static_cast<int>(delta_y);

    if (main_window != nullptr) {
        QPoint snapped;
        QMetaObject::invokeMethod(
            main_window,
            "snap_position",
            Q_RETURN_ARG(QPoint, snapped),
            Q_ARG(QObject *, this),
            Q_ARG(int, new_x),
            Q_ARG(int, new_y),
            Q_ARG(int, 200)
        );
        new_x = snapped.x();
        new_y = snapped.y();
    }

    set_position_x(new_x);
    set_position_y(new_y);
}

void MonitorProperties::activate_preferred_mode() {
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
