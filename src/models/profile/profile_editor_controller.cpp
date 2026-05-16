#include "models/profile/profile_editor_controller.hpp"

#include <qalgorithms.h>
#include <qcontainerfwd.h>
#include <qhashfunctions.h>
#include <qlist.h>
#include <qobject.h>
#include <qpoint.h>
#include <qtmetamacros.h>
#include <spdlog/spdlog.h>

#include <exception>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "backend.hpp"
#include "backend/auto_wlr_randr/pattern_matcher.hpp"
#include "backend/auto_wlr_randr/types.hpp"
#include "backend/profile/editor_backend.hpp"
#include "backend/profile/types.hpp"
#include "models/profile/profile_output_properties.hpp"
#include "monitor_specs.hpp"
#include "utils/helpers.hpp"
#include "utils/string_list_edit.hpp"

ProfileEditorController::ProfileEditorController(BackendManager * backend_manager, QObject * parent)
    : QObject(parent),
      backend_manager_(backend_manager) {
    connect(
        backend_manager_,
        &BackendManager::operation_busy_changed,
        this,
        &ProfileEditorController::operation_busy_changed
    );
    refresh_connected_outputs();

    try {
        working_config_ = profile_backend().load_config();
        saved_config_ = working_config_;
        if (!working_config_.profiles.empty()) {
            selected_profile_id_ = QString::fromStdString(working_config_.profiles.front().id);
            rebuild_outputs_from_profile(working_config_.profiles.front());
        }
    } catch (const std::exception & e) {
        spdlog::warn("Failed to load profile config: {}", e.what());
    }
}

auto ProfileEditorController::get_profile_ids() const -> QStringList {
    QStringList ids;
    for (const profile::ProfileDefinition & profile : working_config_.profiles) {
        ids.append(QString::fromStdString(profile.id));
    }
    return ids;
}

auto ProfileEditorController::get_selected_profile_id() const -> QString {
    return selected_profile_id_;
}

auto ProfileEditorController::is_dirty() const -> bool {
    return dirty_;
}

auto ProfileEditorController::get_active_profile_id() const -> QString {
    return active_profile_id_;
}

auto ProfileEditorController::is_daemon_running() const -> bool {
    return daemon_running_;
}

auto ProfileEditorController::get_match_warning() const -> QString {
    return match_warning_;
}

auto ProfileEditorController::get_daemon_status_text() const -> QString {
    return daemon_status_text_;
}

auto ProfileEditorController::get_save_status_text() const -> QString {
    return save_status_text_;
}

auto ProfileEditorController::get_outputs() const -> QList<QObject *> {
    QList<QObject *> outputs;
    outputs.reserve(output_models_.size());
    for (ProfileOutputProperties * output : output_models_) {
        outputs.append(output);
    }
    return outputs;
}

auto ProfileEditorController::get_exec_commands() const -> QStringList {
    const profile::ProfileDefinition profile = current_profile();
    QStringList commands;
    for (const std::string & command : profile.exec) {
        commands.append(QString::fromStdString(command));
    }
    return commands;
}

auto ProfileEditorController::get_on_no_match_exec_commands() const -> QStringList {
    QStringList commands;
    for (const std::string & command : working_config_.on_no_match_exec) {
        commands.append(QString::fromStdString(command));
    }
    return commands;
}

auto ProfileEditorController::get_connected_outputs() const -> QStringList {
    return connected_outputs_;
}

auto ProfileEditorController::get_connected_output_infos() const
    -> const std::vector<profile::ConnectedOutput> & {
    return connected_output_infos_;
}

auto ProfileEditorController::supports_pattern_matching() const -> bool {
    return capabilities().pattern_matching;
}

auto ProfileEditorController::supports_output_match_preview() const -> bool {
    return capabilities().output_match_preview;
}

auto ProfileEditorController::supports_service_integration() const -> bool {
    return capabilities().service_integration;
}

auto ProfileEditorController::supports_force_switch() const -> bool {
    return capabilities().force_switch;
}

auto ProfileEditorController::supports_on_no_match_exec() const -> bool {
    return capabilities().on_no_match_exec;
}

auto ProfileEditorController::supports_preferred_mode() const -> bool {
    return capabilities().preferred_mode;
}

auto ProfileEditorController::supports_adaptive_sync() const -> bool {
    return capabilities().adaptive_sync;
}

void ProfileEditorController::set_selected_profile_id(const QString & profile_id) {
    select_profile(profile_id, false);
}

auto ProfileEditorController::find_profile(const QString & profile_id)
    -> profile::ProfileDefinition * {
    const std::string profile_id_value = profile_id.toStdString();
    for (profile::ProfileDefinition & profile : working_config_.profiles) {
        if (profile.id == profile_id_value) {
            return &profile;
        }
    }
    return nullptr;
}

auto ProfileEditorController::find_profile(const QString & profile_id) const
    -> const profile::ProfileDefinition * {
    const std::string profile_id_value = profile_id.toStdString();
    for (const profile::ProfileDefinition & profile : working_config_.profiles) {
        if (profile.id == profile_id_value) {
            return &profile;
        }
    }
    return nullptr;
}

auto ProfileEditorController::profile_exists(const QString & profile_id) const -> bool {
    return find_profile(profile_id) != nullptr;
}

auto ProfileEditorController::ensure_selected_profile_exists() -> profile::ProfileDefinition * {
    if (selected_profile_id_.isEmpty()) {
        return nullptr;
    }

    profile::ProfileDefinition * profile = find_profile(selected_profile_id_);
    if (profile != nullptr) {
        return profile;
    }

    profile::ProfileDefinition new_profile;
    new_profile.id = selected_profile_id_.toStdString();
    working_config_.profiles.push_back(std::move(new_profile));
    emit profile_ids_changed();
    return &working_config_.profiles.back();
}

auto ProfileEditorController::select_profile(const QString & profile_id, bool force) -> bool {
    if (backend_manager_->is_operation_busy()) {
        return false;
    }
    if (!force && dirty_ && profile_id != selected_profile_id_) {
        return false;
    }

    const profile::ProfileDefinition * profile = find_profile(profile_id);
    if (profile == nullptr) {
        return false;
    }

    selected_profile_id_ = profile_id;
    rebuild_outputs_from_profile(*profile);
    set_dirty(false);
    emit selected_profile_id_changed();
    update_match_warning();
    return true;
}

bool ProfileEditorController::submit(
    std::function<Completion()> work, std::function<void(const std::string &)> failure
) {
    if (backend_manager_->is_operation_busy() || backend_manager_->has_pending_changes()) {
        return false;
    }
    backend_manager_->set_operation_busy(true);
    auto manager = backend_manager_->shared_from_this();
    if (!failure) {
        failure = [this](const std::string & error) { report_process_error(error); };
    }
    run_job(
        this,
        [manager, work = std::move(work)] { return work(); },
        std::move(failure),
        [manager] { manager->set_operation_busy(false); }
    );
    return true;
}

void ProfileEditorController::report_save_error(const std::string & error) {
    save_status_text_ = QStringLiteral("Configuration not saved: %1. Edits were kept.")
                            .arg(QString::fromStdString(error));
    emit save_status_changed();
}

void ProfileEditorController::report_save_result(const ProfileSaveResult & result) {
    if (!result.reload_error.empty()) {
        save_status_text_ = QStringLiteral("Configuration saved to disk; daemon reload failed: %1")
                                .arg(QString::fromStdString(result.reload_error));
    } else if (result.daemon_reloaded) {
        save_status_text_ = QStringLiteral("Configuration saved to disk; daemon reload succeeded.");
    } else {
        save_status_text_ = QStringLiteral("Configuration saved to disk; daemon is not running.");
    }
    if (dirty_) {
        save_status_text_ += QStringLiteral(" Newer edits remain unsaved.");
    }
    emit save_status_changed();
}

void ProfileEditorController::report_process_error(const std::string & error) {
    daemon_running_ = false;
    active_profile_id_.clear();
    emit active_profile_id_changed();
    daemon_status_text_ = QString::fromStdString(error);
    emit daemon_status_changed();
}

void ProfileEditorController::save_profile() {
    if (selected_profile_id_.isEmpty() || backend_manager_->is_operation_busy() ||
        backend_manager_->has_pending_changes()) {
        return;
    }
    auto config = working_config_;
    const auto profile = current_profile();
    const auto revision = revision_;
    auto * backend = &profile_backend();
    submit(
        [this, backend, config, profile, revision]() mutable {
            backend->add_profile(config, profile);
            const auto result = backend->save_config(config);
            config.file_snapshot = result.file_snapshot;
            return [this, config, revision, result] {
                saved_config_ = config;
                // Newer edits are based on our completed write, not the old disk version.
                working_config_.file_snapshot = config.file_snapshot;
                if (revision == revision_) {
                    working_config_ = config;
                    set_dirty(false);
                }
                emit profile_ids_changed();
                report_save_result(result);
                refresh_daemon_status();
            };
        },
        [this](const std::string & error) { report_save_error(error); }
    );
}

void ProfileEditorController::discard_changes() {
    if (backend_manager_->is_operation_busy()) {
        return;
    }
    working_config_ = saved_config_;
    reload_current_profile_into_editor();
    set_dirty(false);
    emit profile_ids_changed();
    emit on_no_match_exec_commands_changed();
    emit discard_requested();
}

void ProfileEditorController::reload_config_from_disk() {
    if (backend_manager_->is_operation_busy()) {
        return;
    }
    try {
        saved_config_ = profile_backend().load_config();
        working_config_ = saved_config_;
        if (!selected_profile_id_.isEmpty()) {
            reload_current_profile_into_editor();
        } else if (!working_config_.profiles.empty()) {
            selected_profile_id_ = QString::fromStdString(working_config_.profiles.front().id);
            rebuild_outputs_from_profile(working_config_.profiles.front());
            emit selected_profile_id_changed();
        }
        set_dirty(false);
        emit profile_ids_changed();
        emit on_no_match_exec_commands_changed();
        update_match_warning();
    } catch (const std::exception & e) {
        daemon_status_text_ = QString::fromStdString(std::string("Reload failed: ") + e.what());
        emit daemon_status_changed();
    }
}

void ProfileEditorController::on_profile_backend_changed() {
    save_status_text_.clear();
    emit save_status_changed();
    emit capabilities_changed();
    reload_config_from_disk();
    refresh_connected_outputs();
    refresh_daemon_status();
}

void ProfileEditorController::switch_profile(bool force) {
    if (selected_profile_id_.isEmpty()) {
        return;
    }
    const auto id = selected_profile_id_.toStdString();
    auto * backend = &profile_backend();
    submit([this, backend, id, force] {
        backend->switch_profile(id, force);
        return [this] { refresh_connected_outputs(); };
    });
}

void ProfileEditorController::create_profile_from_live(const QString & profile_id) {
    if (profile_id.isEmpty()) {
        return;
    }
    if (profile_exists(profile_id)) {
        daemon_status_text_ = QStringLiteral("Profile already exists: %1").arg(profile_id);
        emit daemon_status_changed();
        return;
    }
    auto * backend = &profile_backend();
    submit([this, backend, profile_id] {
        auto profile = backend->create_profile_from_live(profile_id.toStdString());
        return [this, profile = std::move(profile), profile_id]() mutable {
            if (profile_exists(profile_id)) {
                return;
            }
            profile_backend().add_profile(working_config_, std::move(profile));
            selected_profile_id_ = profile_id;
            reload_current_profile_into_editor();
            set_dirty(true);
            ++revision_;
            emit profile_ids_changed();
            emit selected_profile_id_changed();
        };
    });
}

void ProfileEditorController::duplicate_profile(const QString & source_id, const QString & new_id) {
    if (source_id.isEmpty() || new_id.isEmpty()) {
        return;
    }

    if (profile_exists(new_id)) {
        daemon_status_text_ = QStringLiteral("Profile already exists: %1").arg(new_id);
        emit daemon_status_changed();
        return;
    }

    try {
        profile_backend().duplicate_profile(
            working_config_, source_id.toStdString(), new_id.toStdString()
        );
        selected_profile_id_ = new_id;
        reload_current_profile_into_editor();
        set_dirty(true);
        emit profile_ids_changed();
        emit selected_profile_id_changed();
    } catch (const std::exception & e) {
        daemon_status_text_ = QString::fromStdString(std::string("Duplicate failed: ") + e.what());
        emit daemon_status_changed();
    }
}

void ProfileEditorController::delete_profile(const QString & profile_id) {
    if (profile_id.isEmpty() || backend_manager_->is_operation_busy() ||
        backend_manager_->has_pending_changes()) {
        return;
    }
    auto config = working_config_;
    const auto revision = revision_;
    auto * backend = &profile_backend();
    submit(
        [this, backend, config, profile_id, revision]() mutable {
            backend->delete_profile(config, profile_id.toStdString());
            const auto result = backend->save_config(config);
            config.file_snapshot = result.file_snapshot;
            return [this, config, profile_id, revision, result] {
                saved_config_ = config;
                working_config_.file_snapshot = config.file_snapshot;
                if (revision == revision_) {
                    working_config_ = config;
                    if (selected_profile_id_ == profile_id) {
                        selected_profile_id_.clear();
                        clear_outputs();
                        if (!working_config_.profiles.empty()) {
                            selected_profile_id_ =
                                QString::fromStdString(working_config_.profiles.front().id);
                            rebuild_outputs_from_profile(working_config_.profiles.front());
                        }
                        emit selected_profile_id_changed();
                    }
                    set_dirty(false);
                }
                emit profile_ids_changed();
                report_save_result(result);
                refresh_daemon_status();
            };
        },
        [this](const std::string & error) { report_save_error(error); }
    );
}

void ProfileEditorController::add_output() {
    if (selected_profile_id_.isEmpty()) {
        return;
    }

    if (ensure_selected_profile_exists() == nullptr) {
        return;
    }

    profile::ProfileOutputDefinition output;
    output.output = "HDMI-*";
    output.enabled = true;
    output.mode = "1920x1080";
    output.pos = "0,0";
    output.scale = 1.0F;
    output.transform = "normal";
    output.adaptive_sync = false;

    auto * output_model = new ProfileOutputProperties(this, this);
    output_model->load_from_output(output);
    output_models_.append(output_model);
    mark_dirty();
    emit outputs_changed();
}

void ProfileEditorController::remove_output(int index) {
    if (index < 0 || index >= output_models_.size()) {
        return;
    }

    ProfileOutputProperties * output = output_models_.takeAt(index);
    output->deleteLater();
    mark_dirty();
    emit outputs_changed();
}

void ProfileEditorController::add_exec_command(const QString & command) {
    profile::ProfileDefinition * profile = ensure_selected_profile_exists();
    if (profile == nullptr) {
        return;
    }

    string_list_edit::add(profile->exec, command.toStdString());
    mark_dirty();
    emit exec_commands_changed();
}

void ProfileEditorController::remove_exec_command(int index) {
    profile::ProfileDefinition * profile = ensure_selected_profile_exists();
    if (profile == nullptr) {
        return;
    }

    string_list_edit::remove_at(profile->exec, index);
    mark_dirty();
    emit exec_commands_changed();
}

void ProfileEditorController::set_exec_command(int index, const QString & command) {
    profile::ProfileDefinition * profile = ensure_selected_profile_exists();
    if (profile == nullptr) {
        return;
    }

    string_list_edit::set_at(profile->exec, index, command.toStdString());
    mark_dirty();
    emit exec_commands_changed();
}

void ProfileEditorController::add_on_no_match_exec_command(const QString & command) {
    string_list_edit::add(working_config_.on_no_match_exec, command.toStdString());
    mark_dirty();
    emit on_no_match_exec_commands_changed();
}

void ProfileEditorController::remove_on_no_match_exec_command(int index) {
    string_list_edit::remove_at(working_config_.on_no_match_exec, index);
    mark_dirty();
    emit on_no_match_exec_commands_changed();
}

void ProfileEditorController::set_on_no_match_exec_command(int index, const QString & command) {
    string_list_edit::set_at(working_config_.on_no_match_exec, index, command.toStdString());
    mark_dirty();
    emit on_no_match_exec_commands_changed();
}

void ProfileEditorController::refresh_daemon_status() {
    auto * backend = &profile_backend();
    submit([this, backend] {
        const auto status = backend->service_status();
        return [this, status] {
            daemon_running_ = status.service_running;
            active_profile_id_ = QString::fromStdString(status.active_profile);
            if (daemon_running_) {
                daemon_status_text_ =
                    QStringLiteral("Daemon running · active: %1").arg(active_profile_id_);
            } else {
                daemon_status_text_ = QStringLiteral("Daemon not running");
            }
            emit daemon_status_changed();
            emit active_profile_id_changed();
        };
    });
}

void ProfileEditorController::refresh_connected_outputs() {
    auto * backend = &profile_backend();
    submit([this, backend] {
        auto outputs = backend->get_connected_outputs();
        auto monitors = get_monitor_specs_list();
        return [this, outputs = std::move(outputs), monitors = std::move(monitors)]() mutable {
            connected_output_infos_ = outputs;
            live_monitors_ = std::move(monitors);
            connected_outputs_.clear();
            for (const auto & output : connected_output_infos_) {
                connected_outputs_.append(QString::fromStdString(output.display_label()));
            }
            for (auto * model : output_models_) {
                model->refresh_match_preview();
                model->refresh_live_position();
            }
            update_match_warning();
            emit connected_outputs_changed();
            refresh_daemon_status();
        };
    });
}

auto ProfileEditorController::get_live_position_for_output(const QString & output_pattern) const
    -> QPoint {
    // QML property reads use the last completed snapshot, never spawn a process.
    std::vector<ConnectedOutputInfo> connected;
    connected.reserve(connected_output_infos_.size());
    for (const auto & output : connected_output_infos_) {
        connected.push_back({ output.name, output.make, output.model, output.serial });
    }
    const auto matched =
        AutoWlrRandrPatternMatcher::find_matching_output(output_pattern.toStdString(), connected);
    if (!matched) {
        return { 0, 0 };
    }
    for (const auto & monitor : live_monitors_) {
        if (monitor.get_name() == matched->name && monitor.is_enabled() &&
            monitor.get_enabled_monitor_settings()) {
            const auto & pos = monitor.get_enabled_monitor_settings()->get_position();
            return { pos.x, pos.y };
        }
    }
    return { 0, 0 };
}

auto ProfileEditorController::get_output_index(QObject * output) const -> int {
    for (int index = 0; index < output_models_.size(); ++index) {
        if (output_models_[index] == output) {
            return index;
        }
    }
    return -1;
}

void ProfileEditorController::mark_dirty() {
    ++revision_;
    if (selected_profile_id_.isEmpty()) {
        return;
    }

    profile::ProfileDefinition updated_profile;
    updated_profile.id = selected_profile_id_.toStdString();
    for (ProfileOutputProperties const * output_obj : output_models_) {
        updated_profile.outputs.push_back(output_obj->to_output());
    }

    profile::ProfileDefinition * existing_profile = find_profile(selected_profile_id_);
    if (existing_profile != nullptr) {
        updated_profile.exec = existing_profile->exec;
        *existing_profile = std::move(updated_profile);
    } else {
        working_config_.profiles.push_back(std::move(updated_profile));
        emit profile_ids_changed();
    }

    set_dirty(true);
    update_match_warning();
}

void ProfileEditorController::reload_current_profile_into_editor() {
    if (selected_profile_id_.isEmpty()) {
        clear_outputs();
        return;
    }

    const profile::ProfileDefinition * profile = find_profile(selected_profile_id_);
    if (profile == nullptr) {
        clear_outputs();
        return;
    }

    rebuild_outputs_from_profile(*profile);
    update_match_warning();
}

void ProfileEditorController::clear_outputs() {
    qDeleteAll(output_models_);
    output_models_.clear();
    emit outputs_changed();
}

void ProfileEditorController::rebuild_outputs_from_profile(
    const profile::ProfileDefinition & profile
) {
    clear_outputs();
    for (const profile::ProfileOutputDefinition & output : profile.outputs) {
        auto * output_model = new ProfileOutputProperties(this, this);
        output_model->load_from_output(output);
        output_models_.append(output_model);
    }
    emit outputs_changed();
    emit exec_commands_changed();
}

auto ProfileEditorController::current_profile() const -> profile::ProfileDefinition {
    const profile::ProfileDefinition * profile = find_profile(selected_profile_id_);
    if (profile == nullptr) {
        return {};
    }
    return *profile;
}

void ProfileEditorController::update_match_warning() {
    const profile::ProfileDefinition profile = current_profile();
    const std::string warning =
        profile_backend().get_match_warning(profile, connected_output_infos_);
    match_warning_ = QString::fromStdString(warning);
    emit match_warning_changed();
}

auto ProfileEditorController::profile_backend() const -> const ProfileEditorBackend & {
    return backend_manager_->profile_editor();
}

auto ProfileEditorController::profile_backend() -> ProfileEditorBackend & {
    return backend_manager_->profile_editor();
}

auto ProfileEditorController::capabilities() const -> profile::ProfileEditorCapabilities {
    return profile_backend().capabilities();
}

auto ProfileEditorController::get_output_match_preview(const std::string & output_selector) const
    -> QString {
    return QString::fromStdString(
        profile_backend().describe_output_match(output_selector, connected_output_infos_)
    );
}

void ProfileEditorController::set_dirty(bool dirty) {
    ++revision_;
    if (dirty_ == dirty) {
        return;
    }
    dirty_ = dirty;
    emit is_dirty_changed();
}
