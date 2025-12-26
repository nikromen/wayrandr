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
#include <unordered_map>

#include "backend.hpp"
#include "backend/base.hpp"
#include "models/monitor_block.hpp"
#include "models/monitor_properties.hpp"
#include "monitor_specs.hpp"

MainWindow::MainWindow(QObject * parent)
    : QObject(parent),
      confirmation_timer_(new QTimer(this)),
      countdown_timer_(new QTimer(this)) {
    spdlog::debug("Initializing MainWindow and loading monitor specifications");
    monitors_ = get_monitor_specs_list();
    spdlog::debug("Found {} monitors", monitors_.size());

    for (auto & monitor_spec : monitors_) {
        spdlog::debug("Creating model for monitor: {}", monitor_spec.get_name());
        auto * monitor_props = new MonitorProperties(&monitor_spec, this);
        monitors_models_.append(monitor_props);

        QString const monitor_name = QString::fromStdString(monitor_spec.get_name());
        auto * block = new MonitorBlock(monitor_name, this);
        monitor_blocks_[monitor_name] = block;

        if (monitor_spec.is_enabled()) {
            block->start_capture();
        }

        connect(monitor_props, &MonitorProperties::enabled_changed, this, [this, block, monitor_props]() {
            if (monitor_props->is_enabled()) {
                block->start_capture();
            } else {
                block->stop_capture();
            }
        });
    }

    backend_manager_ = std::make_unique<BackendManager>();

    confirmation_timer_->setSingleShot(true);
    connect(confirmation_timer_, &QTimer::timeout, this, &MainWindow::on_confirmation_timeout);

    countdown_timer_->setInterval(1000);
    connect(countdown_timer_, &QTimer::timeout, this, &MainWindow::on_countdown_tick);
}

auto MainWindow::get_monitors() const -> QList<QObject *> {
    return monitors_models_;
}

auto MainWindow::is_confirmation_pending() const -> bool {
    return confirmation_pending_;
}

auto MainWindow::get_confirmation_seconds_left() const -> int {
    return confirmation_seconds_left_;
}

auto MainWindow::get_monitor_block(const QString & monitor_name) -> QObject * {
    if (monitor_blocks_.contains(monitor_name)) {
        return monitor_blocks_[monitor_name];
    }
    return nullptr;
}

void MainWindow::apply() {
    if (confirmation_pending_) {
        spdlog::warn("Apply requested while confirmation is already pending");
        return;
    }

    spdlog::info(
        "Applying monitor configuration changes for {} monitors", monitors_.size()
    );

    try {
        backend_manager_->apply_with_confirmation(
            monitors_, nullptr, kConfirmationTimeoutSeconds
        );
        set_confirmation_pending(true);
        set_confirmation_seconds_left(kConfirmationTimeoutSeconds);
        confirmation_timer_->start(kConfirmationTimeoutSeconds * 1000);
        countdown_timer_->start();
        spdlog::info("Confirmation timer started ({} seconds)", kConfirmationTimeoutSeconds);
    } catch (const std::exception & e) {
        spdlog::error("Failed to apply configuration: {}", e.what());
    }
}

void MainWindow::save() {
    spdlog::info("Saving monitor configuration to persistent storage");
}

void MainWindow::confirm_apply() {
    spdlog::info("User confirmed configuration");
    confirmation_timer_->stop();
    countdown_timer_->stop();
    set_confirmation_pending(false);
    backend_manager_->confirm_apply();
    reload_monitors();
}

void MainWindow::cancel_apply() {
    spdlog::info("User cancelled configuration");
    confirmation_timer_->stop();
    countdown_timer_->stop();
    set_confirmation_pending(false);
    backend_manager_->cancel_apply();
    reload_monitors();
}

void MainWindow::on_confirmation_timeout() {
    spdlog::warn("Configuration confirmation timeout - reverting");
    countdown_timer_->stop();
    set_confirmation_pending(false);
    backend_manager_->cancel_apply();
    reload_monitors();
}

void MainWindow::on_countdown_tick() {
    if (!confirmation_pending_) {
        countdown_timer_->stop();
        return;
    }

    const int remaining_ms = confirmation_timer_->remainingTime();
    const int seconds_left = std::max(0, (remaining_ms + 999) / 1000);
    set_confirmation_seconds_left(seconds_left);
}

void MainWindow::set_backend(int backend_type) {
    if (backend_type != static_cast<int>(BackendType::WLR_RANDR)) {
        spdlog::warn("Backend type {} is not supported yet, keeping wlr-randr", backend_type);
        return;
    }

    spdlog::info("Setting backend type to wlr-randr");
    backend_manager_->set_backend(BackendType::WLR_RANDR);
}

void MainWindow::reload_monitors() {
    const auto fresh_specs = get_monitor_specs_list();
    std::unordered_map<std::string, const MonitorSpecs *> fresh_by_name;
    for (const auto & spec : fresh_specs) {
        fresh_by_name[spec.get_name()] = &spec;
    }

    int synced = 0;
    for (auto & monitor_spec : monitors_) {
        const auto it = fresh_by_name.find(monitor_spec.get_name());
        if (it == fresh_by_name.end()) {
            continue;
        }

        monitor_spec.sync_from(*it->second);
        synced++;
    }

    for (auto * monitor_obj : monitors_models_) {
        auto * monitor_props = qobject_cast<MonitorProperties *>(monitor_obj);
        if (monitor_props != nullptr) {
            monitor_props->notify_all_changed();
        }
    }

    for (auto it = monitor_blocks_.begin(); it != monitor_blocks_.end(); ++it) {
        const auto spec_it = fresh_by_name.find(it.key().toStdString());
        if (spec_it == fresh_by_name.end()) {
            it.value()->stop_capture();
            continue;
        }

        if (spec_it->second->is_enabled()) {
            it.value()->start_capture();
        } else {
            it.value()->stop_capture();
        }
    }

    spdlog::info("Reloaded {} monitors from wlr-randr", synced);
}

void MainWindow::set_confirmation_pending(bool pending) {
    if (confirmation_pending_ == pending) {
        return;
    }

    confirmation_pending_ = pending;
    emit confirmation_pending_changed();
}

void MainWindow::set_confirmation_seconds_left(int seconds) {
    if (confirmation_seconds_left_ == seconds) {
        return;
    }

    confirmation_seconds_left_ = seconds;
    emit confirmation_seconds_left_changed();
}

auto MainWindow::snap_position(
    QObject * monitor,
    int x,
    int y,
    int snap_threshold,
    int canvas_width,
    int canvas_height,
    float display_scale
) const -> QPoint {
    auto * current_monitor = qobject_cast<MonitorProperties *>(monitor);
    if ((current_monitor == nullptr) || !current_monitor->has_settings()) {
        spdlog::debug("snap_position: invalid monitor, returning input position");
        return { x, y };
    }

    const int layout_width = current_monitor->get_layout_width();
    const int layout_height = current_monitor->get_layout_height();

    int snapped_x = std::max(0, x);
    int snapped_y = std::max(0, y);

    int const this_left = snapped_x;
    int const this_right = snapped_x + layout_width;
    int const this_top = snapped_y;
    int const this_bottom = snapped_y + layout_height;

    for (auto * other_obj : monitors_models_) {
        auto * other = qobject_cast<MonitorProperties *>(other_obj);
        if ((other == nullptr) || other == current_monitor || !other->is_enabled() ||
            !other->has_settings()) {
            continue;
        }

        const int other_layout_width = other->get_layout_width();
        const int other_layout_height = other->get_layout_height();

        int const other_left = other->get_position_x();
        int const other_right = other->get_position_x() + other_layout_width;
        int const other_top = other->get_position_y();
        int const other_bottom = other->get_position_y() + other_layout_height;

        if (std::abs(this_left - other_right) < snap_threshold) {
            snapped_x = other_right;
        } else if (std::abs(this_right - other_left) < snap_threshold) {
            snapped_x = other_left - layout_width;
        } else if (std::abs(this_left - other_left) < snap_threshold) {
            snapped_x = other_left;
        } else if (std::abs(this_right - other_right) < snap_threshold) {
            snapped_x = other_right - layout_width;
        }

        if (std::abs(this_top - other_bottom) < snap_threshold) {
            snapped_y = other_bottom;
        } else if (std::abs(this_bottom - other_top) < snap_threshold) {
            snapped_y = other_top - layout_height;
        } else if (std::abs(this_top - other_top) < snap_threshold) {
            snapped_y = other_top;
        } else if (std::abs(this_bottom - other_bottom) < snap_threshold) {
            snapped_y = other_bottom - layout_height;
        }
    }

    snapped_x = std::max(0, snapped_x);
    snapped_y = std::max(0, snapped_y);

    if (canvas_width > 0 && canvas_height > 0 && display_scale > 0.0F) {
        const int max_x = std::max(
            0,
            static_cast<int>(canvas_width / display_scale) - layout_width
        );
        const int max_y = std::max(
            0,
            static_cast<int>(canvas_height / display_scale) - layout_height
        );
        snapped_x = std::min(snapped_x, max_x);
        snapped_y = std::min(snapped_y, max_y);
    }

    return { snapped_x, snapped_y };
}

void MainWindow::reset_canvas_layout(int /*canvas_width*/, float /*display_scale*/) {
    spdlog::info("Resetting canvas layout");

    int x = 0;
    const int y = 0;

    for (auto * monitor_obj : monitors_models_) {
        auto * monitor = qobject_cast<MonitorProperties *>(monitor_obj);
        if ((monitor == nullptr) || !monitor->is_enabled() || !monitor->has_settings()) {
            continue;
        }

        monitor->set_position_x(x);
        monitor->set_position_y(y);
        x += monitor->get_layout_width();
    }
}

auto MainWindow::get_monitor_index(QObject * monitor) const -> int {
    for (int i = 0; i < monitors_models_.size(); ++i) {
        if (monitors_models_[i] == monitor) {
            return i;
        }
    }
    return -1;
}
