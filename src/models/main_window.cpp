#include "models/main_window.hpp"

#include <qhashfunctions.h>
#include <qlist.h>
#include <qobject.h>
#include <qtimer.h>
#include <qtmetamacros.h>
#include <spdlog/spdlog.h>

#include <QVariantMap>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "backend.hpp"
#include "backend/base.hpp"
#include "models/monitor_block.hpp"
#include "models/monitor_properties.hpp"
#include "models/profile/profile_editor_controller.hpp"
#include "models/profile/profile_output_properties.hpp"
#include "models/shared/canvas_participants.hpp"
#include "monitor_specs.hpp"
#include "utils/canvas_layout.hpp"
#include "utils/helpers.hpp"

MainWindow::MainWindow(QObject * parent)
    : QObject(parent),
      confirmation_timer_(new QTimer(this)),
      countdown_timer_(new QTimer(this)) {
    spdlog::debug("Initializing MainWindow and loading monitor specifications");
    kanshi_available_ = is_program_available("kanshi") && is_program_available("kanshictl");
    auto_wlr_randr_available_ =
        is_program_available("auto-wlr-randr") && is_program_available("auto-wlr-randrctl");
    spdlog::info(
        "Optional backends available: kanshi={}, auto-wlr-randr={}",
        kanshi_available_,
        auto_wlr_randr_available_
    );

    backend_manager_ = std::make_shared<BackendManager>();
    connect(
        backend_manager_.get(),
        &BackendManager::operation_busy_changed,
        this,
        &MainWindow::confirmation_pending_changed
    );
    backend_manager_->set_operation_busy(true);
    profile_editor_ = std::make_unique<ProfileEditorController>(backend_manager_.get(), this);

    confirmation_timer_->setSingleShot(true);
    connect(confirmation_timer_, &QTimer::timeout, this, &MainWindow::on_confirmation_timeout);

    countdown_timer_->setInterval(1000);
    connect(countdown_timer_, &QTimer::timeout, this, &MainWindow::on_countdown_tick);
    run_job(
        this,
        [this] {
            auto specs = get_monitor_specs_list();
            return [this, specs = std::move(specs)] {
                backend_manager_->set_operation_busy(false);
                initialize_monitors(specs);
                if (close_requested_) {
                    finish_close();
                }
            };
        },
        [this](const std::string & error) {
            backend_manager_->set_operation_busy(false);
            initialized_ = true;
            emit confirmation_pending_changed();
            set_apply_error(QString::fromStdString(error));
            if (close_requested_) {
                finish_close();
            }
        }
    );
}

void MainWindow::finish_close() {
    close_requested_ = false;
    cancel_requested_ = false;
    emit close_ready();
}

MainWindow::~MainWindow() {
    // Queue recovery behind any in-flight Apply. The job owns the manager even
    // if the window is destroyed before delivery of its result. Receiverless jobs
    // survive shutdown_commands(), which drains recovery before stopping the worker.
    const auto manager = backend_manager_;
    run_job(
        nullptr,
        [manager] {
            if (manager->has_pending_changes()) {
                manager->cancel_apply();
                spdlog::info("Previous monitor configuration restored after window destruction");
            }
            return Completion{};
        },
        [](const std::string &) {}
    );
}

void MainWindow::initialize_monitors(const std::vector<MonitorSpecs> & specs) {
    reload_monitors(specs);
    initialized_ = true;
    emit confirmation_pending_changed();
    if (!close_requested_) {
        profile_editor_->refresh_connected_outputs();
    }
}

auto MainWindow::create_monitor(const MonitorSpecs & specs) -> MonitorProperties * {
    auto * model = new MonitorProperties(specs, this);
    auto * block = new MonitorBlock(model->get_name(), this);
    monitor_blocks_.insert(model->get_name(), block);
    connect(model, &MonitorProperties::enabled_changed, this, [model, block] {
        if (model->is_enabled()) {
            block->start_capture();
        } else {
            block->stop_capture();
        }
    });
    return model;
}

auto MainWindow::get_monitors() const -> QList<QObject *> {
    return monitors_models_;
}

auto MainWindow::get_backend_mode() const -> int {
    return backend_mode_;
}

auto MainWindow::get_profile_editor() const -> ProfileEditorController * {
    return profile_editor_.get();
}

auto MainWindow::is_confirmation_pending() const -> bool {
    return confirmation_pending_;
}

auto MainWindow::get_confirmation_seconds_left() const -> int {
    return confirmation_seconds_left_;
}

auto MainWindow::is_kanshi_available() const -> bool {
    return kanshi_available_;
}

auto MainWindow::is_auto_wlr_randr_available() const -> bool {
    return auto_wlr_randr_available_;
}

auto MainWindow::get_monitor_block(const QString & monitor_name) -> QObject * {
    if (monitor_blocks_.contains(monitor_name)) {
        return monitor_blocks_[monitor_name];
    }
    return nullptr;
}

void MainWindow::apply() {
    if (!initialized_ || backend_manager_->is_operation_busy() ||
        backend_mode_ != K_BACKEND_MODE_WLR_RANDR || backend_manager_->has_pending_changes()) {
        return;
    }
    backend_manager_->set_operation_busy(true);
    set_apply_error({});
    const auto manager = backend_manager_;
    std::vector<MonitorSpecs> monitors;
    for (auto * object : monitors_models_) {
        const auto * model = qobject_cast<MonitorProperties *>(object);
        if (model->is_connected()) {
            monitors.push_back(model->get_specs());
        }
    }
    run_job(
        this,
        [this, manager, monitors] {
            manager->apply_with_confirmation(monitors, nullptr, K_CONFIRMATION_TIMEOUT_SECONDS);
            return [this] {
                for (auto * object : monitors_models_) {
                    auto * model = qobject_cast<MonitorProperties *>(object);
                    if (model->is_connected()) {
                        model->mark_clean();
                    }
                }
                sync_confirmation_state();
                confirmation_timer_->start(K_CONFIRMATION_TIMEOUT_SECONDS * 1000);
                countdown_timer_->start();
                set_confirmation_seconds_left(K_CONFIRMATION_TIMEOUT_SECONDS);
                if (cancel_requested_ || close_requested_) {
                    cancel_requested_ = false;
                    start_restore();
                    return;
                }
                // Reentrant notify handlers may Confirm/Cancel as soon as the gate opens.
                backend_manager_->set_operation_busy(false);
            };
        },
        [this](const std::string & error) {
            set_apply_error(QString::fromStdString(error));
            stop_confirmation_timers();
            sync_confirmation_state();
            cancel_requested_ = false;
            if (!backend_manager_->has_pending_changes()) {
                if (close_requested_) {
                    backend_manager_->set_operation_busy(false);
                    finish_close();
                } else {
                    reload_monitors_safely();
                }
            } else {
                close_requested_ = false;
                backend_manager_->set_operation_busy(false);
            }
        }
    );
}

void MainWindow::save() {
    spdlog::info("Saving monitor configuration to persistent storage");
}

auto MainWindow::is_confirmation_allowed() const -> bool {
    return !backend_manager_->is_operation_busy() && backend_manager_->can_confirm();
}

auto MainWindow::get_apply_error() const -> QString {
    return apply_error_;
}

void MainWindow::set_apply_error(const QString & error) {
    apply_error_ = error;
    if (!error.isEmpty()) {
        spdlog::error("{}", error.toStdString());
    }
    emit apply_error_changed();
}

void MainWindow::sync_confirmation_state() {
    const bool pending = backend_manager_->has_pending_changes();
    if (pending == confirmation_pending_) {
        // can_confirm may change while the snapshot remains pending.
        emit confirmation_pending_changed();
    } else {
        set_confirmation_pending(pending);
    }
}

void MainWindow::stop_confirmation_timers() {
    confirmation_timer_->stop();
    countdown_timer_->stop();
    set_confirmation_seconds_left(0);
}

void MainWindow::reload_monitors_safely(bool preserve_drafts) {
    // Continue the admitted operation without reopening editing before refresh.
    backend_manager_->set_operation_busy(true);
    run_job(
        this,
        [this, preserve_drafts] {
            auto specs = get_monitor_specs_list();
            return [this, specs = std::move(specs), preserve_drafts] {
                reload_monitors(specs, preserve_drafts);
                // Refresh has no pending recovery. Consume its Cancel before admitting
                // another Apply, which may start synchronously from the unlock notification.
                cancel_requested_ = false;
                backend_manager_->set_operation_busy(false);
                if (close_requested_) {
                    finish_close();
                }
            };
        },
        [this](const std::string & error) {
            set_apply_error(
                apply_error_ + "\nCould not refresh monitors: " + QString::fromStdString(error)
            );
            cancel_requested_ = false;
            backend_manager_->set_operation_busy(false);
            if (close_requested_) {
                finish_close();
            }
        }
    );
}

void MainWindow::confirm_apply() {
    if (!is_confirmation_allowed()) {
        return;
    }
    backend_manager_->set_operation_busy(true);
    backend_manager_->confirm_apply();
    stop_confirmation_timers();
    sync_confirmation_state();
    set_apply_error({});
    reload_monitors_safely();
}

void MainWindow::cancel_apply() {
    if (backend_manager_->is_operation_busy()) {
        cancel_requested_ = true;
        stop_confirmation_timers();
        return;
    }
    if (!backend_manager_->has_pending_changes()) {
        stop_confirmation_timers();
        if (close_requested_) {
            finish_close();
        }
        return;
    }
    backend_manager_->set_operation_busy(true);
    start_restore();
}

void MainWindow::start_restore() {
    // Both an explicit Cancel and an Apply with queued Cancel/close arrive with the gate
    // held. Hand off to recovery without exposing an intermediate confirmation state.
    stop_confirmation_timers();
    const auto manager = backend_manager_;
    run_job(
        this,
        [this, manager] {
            manager->cancel_apply();
            return [this] {
                cancel_requested_ = false;
                set_apply_error({});
                sync_confirmation_state();
                if (close_requested_) {
                    backend_manager_->set_operation_busy(false);
                    finish_close();
                } else {
                    reload_monitors_safely();
                }
            };
        },
        [this](const std::string & error) {
            close_requested_ = false;
            cancel_requested_ = false;
            set_apply_error("Restore failed. Retry: " + QString::fromStdString(error));
            sync_confirmation_state();
            backend_manager_->set_operation_busy(false);
        }
    );
}

auto MainWindow::prepare_close() -> bool {
    if (backend_manager_->is_operation_busy() || backend_manager_->has_pending_changes()) {
        close_requested_ = true;
        if (backend_manager_->is_operation_busy()) {
            QTimer::singleShot(50, this, [this] {
                if (close_requested_ && prepare_close()) {
                    finish_close();
                }
            });
        }
        cancel_apply();
        return false;
    }
    return true;
}

void MainWindow::on_confirmation_timeout() {
    cancel_apply();
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
    set_backend_mode(backend_type);
}

void MainWindow::set_backend_mode(int backend_mode) {
    if (!can_switch_backend(backend_mode)) {
        return;
    }
    set_backend_mode_internal(backend_mode);
}

auto MainWindow::can_switch_backend(int backend_mode) const -> bool {
    if (backend_manager_->is_operation_busy()) {
        return false;
    }
    if (backend_mode == backend_mode_) {
        return true;
    }

    if ((backend_mode_ == K_BACKEND_MODE_AUTO_WLR_RANDR ||
         backend_mode_ == K_BACKEND_MODE_KANSHI) &&
        backend_mode != backend_mode_ && profile_editor_ != nullptr &&
        profile_editor_->is_dirty()) {
        return false;
    }

    if (backend_manager_->has_pending_changes()) {
        return false;
    }

    if (backend_mode == K_BACKEND_MODE_KANSHI && !kanshi_available_) {
        return false;
    }

    if (backend_mode == K_BACKEND_MODE_AUTO_WLR_RANDR && !auto_wlr_randr_available_) {
        return false;
    }

    return backend_mode == K_BACKEND_MODE_WLR_RANDR || backend_mode == K_BACKEND_MODE_KANSHI ||
        backend_mode == K_BACKEND_MODE_AUTO_WLR_RANDR;
}

void MainWindow::set_backend_mode_internal(int backend_mode) {
    if (backend_mode_ == backend_mode) {
        return;
    }

    // Admit the return refresh before any mode notify can allow Apply on stale models.
    if (backend_mode == K_BACKEND_MODE_WLR_RANDR) {
        backend_manager_->set_operation_busy(true);
        set_apply_error({});
    }
    backend_mode_ = backend_mode;
    if (backend_mode == K_BACKEND_MODE_WLR_RANDR) {
        backend_manager_->set_backend(BackendType::WLR_RANDR);
    } else if (backend_mode == K_BACKEND_MODE_KANSHI) {
        backend_manager_->set_backend(BackendType::KANSHI);
        if (profile_editor_ != nullptr) {
            profile_editor_->on_profile_backend_changed();
        }
    } else if (backend_mode == K_BACKEND_MODE_AUTO_WLR_RANDR) {
        backend_manager_->set_backend(BackendType::AUTO_WLR_RANDR);
        if (profile_editor_ != nullptr) {
            profile_editor_->on_profile_backend_changed();
        }
    } else {
        spdlog::warn("Backend mode {} is not supported yet", backend_mode);
        return;
    }

    spdlog::info("Switched UI backend mode to {}", backend_mode);
    emit backend_mode_changed();
    if (backend_mode == K_BACKEND_MODE_WLR_RANDR) {
        reload_monitors_safely(true);
    }
}

void MainWindow::reload_monitors(
    const std::vector<MonitorSpecs> & fresh_specs, bool preserve_drafts
) {
    // Validate identity before changing any GUI state. Empty arrays are real disconnects;
    // query/decoding errors are handled by the caller and never reach reconciliation.
    std::unordered_set<std::string> names;
    for (const auto & spec : fresh_specs) {
        if (spec.get_name().empty() || !names.insert(spec.get_name()).second) {
            throw std::runtime_error("Invalid or duplicate output name in wlr-randr snapshot");
        }
    }
    QList<QObject *> models;
    for (const auto & spec : fresh_specs) {
        const auto name = QString::fromStdString(spec.get_name());
        const auto found = std::find_if(
            monitors_models_.cbegin(), monitors_models_.cend(), [&name](const auto * object) {
                return qobject_cast<const MonitorProperties *>(object)->get_name() == name;
            }
        );
        MonitorProperties * model = nullptr;
        if (found == monitors_models_.cend()) {
            model = create_monitor(spec);
        } else {
            model = qobject_cast<MonitorProperties *>(*found);
            model->refresh_from(spec, preserve_drafts);
        }
        model->set_connected(true);
        models.append(model);
        auto * block = monitor_blocks_.value(name);
        if (spec.is_enabled()) {
            block->start_capture();
        } else {
            block->stop_capture();
        }
    }
    for (auto * object : monitors_models_) {
        if (!models.contains(object)) {
            auto * model = qobject_cast<MonitorProperties *>(object);
            monitor_blocks_.value(model->get_name())->stop_capture();
            model->set_connected(false);
            models.append(model);
        }
    }
    if (models != monitors_models_) {
        monitors_models_ = std::move(models);
        emit monitors_changed();
    }
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
    QObject * item,
    int x,
    int y,
    int snap_threshold,
    int canvas_width,
    int canvas_height,
    float display_scale
) const -> QPoint {
    if ((backend_mode_ == K_BACKEND_MODE_AUTO_WLR_RANDR ||
         backend_mode_ == K_BACKEND_MODE_KANSHI) &&
        profile_editor_ != nullptr) {
        auto * current_output = qobject_cast<ProfileOutputProperties *>(item);
        if (current_output == nullptr) {
            return { x, y };
        }

        const canvas_participants::SnapContext context =
            canvas_participants::build_profile_snap_context(current_output, profile_editor_.get());
        if (context.rects.empty()) {
            return { x, y };
        }

        const canvas_layout::CanvasRect & current_rect = context.rects[context.current_index];
        canvas_layout::CanvasRect moving_rect = current_rect;
        moving_rect.x = x;
        moving_rect.y = y;
        moving_rect.active = current_output->is_enabled();

        return canvas_layout::snap_from_rects(
            moving_rect,
            context.rects,
            context.current_index,
            x,
            y,
            snap_threshold,
            canvas_width,
            canvas_height,
            display_scale
        );
    }

    auto * current_monitor = qobject_cast<MonitorProperties *>(item);
    if ((current_monitor == nullptr) || !current_monitor->has_settings()) {
        spdlog::debug("snap_position: invalid monitor, returning input position");
        return { x, y };
    }

    const canvas_participants::SnapContext context =
        canvas_participants::build_monitor_snap_context(current_monitor, monitors_models_);
    if (context.rects.empty()) {
        return { x, y };
    }

    const canvas_layout::CanvasRect moving_rect = {
        x, y, current_monitor->get_layout_width(), current_monitor->get_layout_height(), true,
    };

    return canvas_layout::snap_from_rects(
        moving_rect,
        context.rects,
        context.current_index,
        x,
        y,
        snap_threshold,
        canvas_width,
        canvas_height,
        display_scale
    );
}

void MainWindow::reset_canvas_layout(int /*canvas_width*/, float /*display_scale*/) {
    if (backend_mode_ == K_BACKEND_MODE_WLR_RANDR && !is_direct_editing_allowed()) {
        return;
    }
    spdlog::info("Resetting canvas layout");

    if ((backend_mode_ == K_BACKEND_MODE_AUTO_WLR_RANDR ||
         backend_mode_ == K_BACKEND_MODE_KANSHI) &&
        profile_editor_ != nullptr) {
        canvas_participants::ResetContext context =
            canvas_participants::build_profile_reset_context(profile_editor_.get());
        canvas_layout::reset_horizontal_layout(context.items);
        canvas_participants::apply_profile_reset(profile_editor_.get(), context);
        return;
    }

    canvas_participants::ResetContext context =
        canvas_participants::build_monitor_reset_context(monitors_models_);
    canvas_layout::reset_horizontal_layout(context.items);
    canvas_participants::apply_monitor_reset(monitors_models_, context);
}

auto MainWindow::get_monitor_index(QObject * monitor) const -> int {
    for (int i = 0; i < monitors_models_.size(); ++i) {
        if (monitors_models_[i] == monitor) {
            return i;
        }
    }
    return -1;
}
