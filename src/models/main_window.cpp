#include "models/main_window.hpp"

#include <qlist.h>
#include <qobject.h>
#include <qtimer.h>
#include <qtmetamacros.h>
#include <spdlog/spdlog.h>

#include <QVariantMap>
#include <algorithm>
#include <cstdlib>
#include <memory>
#include <stdexcept>

#include "backend.hpp"
#include "backend/base.hpp"
#include "models/monitor_block.hpp"
#include "models/monitor_properties.hpp"
#include "monitor_specs.hpp"

MainWindow::MainWindow(QObject * parent)
    : QObject(parent),
      confirmation_timer_(new QTimer(this)) {
    spdlog::debug("Initializing MainWindow and loading monitor specifications");
    monitors_ = get_monitor_specs_list();
    spdlog::debug("Found {} monitors", monitors_.size());

    for (auto & monitor_spec : monitors_) {
        spdlog::debug("Creating model for monitor: {}", monitor_spec.get_name());
        auto * monitor_props = new MonitorProperties(&monitor_spec, this);
        monitors_models_.append(monitor_props);

        // Create monitor block for preview
        QString const monitor_name = QString::fromStdString(monitor_spec.get_name());
        auto * block = new MonitorBlock(monitor_name, monitor_props, this);
        monitor_blocks_[monitor_name] = block;

        // Start capture if monitor is enabled
        if (monitor_spec.is_enabled()) {
            block->start_capture();
        }
    }

    backend_manager_ = std::make_unique<BackendManager>();


    confirmation_timer_->setSingleShot(true);
    connect(confirmation_timer_, &QTimer::timeout, this, &MainWindow::on_confirmation_timeout);
}

auto MainWindow::get_monitors() const -> QList<QObject *> {
    return monitors_models_;
}

auto MainWindow::get_monitor_block(const QString & monitor_name) -> QObject * {
    if (monitor_blocks_.contains(monitor_name)) {
        return monitor_blocks_[monitor_name];
    }
    return nullptr;
}

void MainWindow::apply() {
    spdlog::info("Applying monitor configuration changes");
    // Implement actual application logic here
}

void MainWindow::save() {
    spdlog::info("Saving monitor configuration to persistent storage");

    // TODO: save will be available only for profile backends not for wlr randr
}

void MainWindow::confirm_apply() {
    spdlog::info("User confirmed configuration");
    confirmation_timer_->stop();
    backend_manager_->confirm_apply();
}

void MainWindow::cancel_apply() {
    spdlog::info("User cancelled configuration");
    confirmation_timer_->stop();
    backend_manager_->cancel_apply();
}

void MainWindow::on_confirmation_timeout() {
    spdlog::warn("Configuration confirmation timeout - reverting");
    backend_manager_->cancel_apply();
}

void MainWindow::set_backend(int backend_type) {
    spdlog::info("Setting backend type to {}", backend_type);
    backend_manager_->set_backend(static_cast<BackendType>(backend_type));
}

auto MainWindow::snap_position(QObject * monitor, int x, int y, int snap_threshold) const
    -> QPoint {
    auto * current_monitor = qobject_cast<MonitorProperties *>(monitor);
    if ((current_monitor == nullptr) || !current_monitor->has_settings()) {
        throw std::invalid_argument("Invalid monitor object or monitor has no settings");
    }

    int snapped_x = std::max(0, x);
    int snapped_y = std::max(0, y);

    int const this_left = snapped_x;
    int const this_right = snapped_x + current_monitor->get_resolution_width();
    int const this_top = snapped_y;
    int const this_bottom = snapped_y + current_monitor->get_resolution_height();

    // check against all other monitors
    for (auto * other_obj : monitors_models_) {
        auto * other = qobject_cast<MonitorProperties *>(other_obj);
        if ((other == nullptr) || other == current_monitor || !other->is_enabled() ||
            !other->has_settings()) {
            continue;
        }

        int const other_left = other->get_position_x();
        int const other_right = other->get_position_x() + other->get_resolution_width();
        int const other_top = other->get_position_y();
        int const other_bottom = other->get_position_y() + other->get_resolution_height();

        // horizontal edges
        if (std::abs(this_left - other_right) < snap_threshold) {
            snapped_x = other_right;
        } else if (std::abs(this_right - other_left) < snap_threshold) {
            snapped_x = other_left - current_monitor->get_resolution_width();
        } else if (std::abs(this_left - other_left) < snap_threshold) {
            snapped_x = other_left;
        } else if (std::abs(this_right - other_right) < snap_threshold) {
            snapped_x = other_right - current_monitor->get_resolution_width();
        }

        // vertical edges
        if (std::abs(this_top - other_bottom) < snap_threshold) {
            snapped_y = other_bottom;
        } else if (std::abs(this_bottom - other_top) < snap_threshold) {
            snapped_y = other_top - current_monitor->get_resolution_height();
        } else if (std::abs(this_top - other_top) < snap_threshold) {
            snapped_y = other_top;
        } else if (std::abs(this_bottom - other_bottom) < snap_threshold) {
            snapped_y = other_bottom - current_monitor->get_resolution_height();
        }
    }

    // ensure final position is not negative
    return { std::max(0, snapped_x), std::max(0, snapped_y) };
}

auto MainWindow::get_monitor_index(QObject * monitor) const -> int {
    for (int i = 0; i < monitors_models_.size(); ++i) {
        if (monitors_models_[i] == monitor) {
            return i;
        }
    }
    return -1;
}
