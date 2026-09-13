#include "models/profile/profile_editor_controller.hpp"

#include <spdlog/spdlog.h>

#include <string>
#include <utility>

#include "backend/auto_wlr_randr/pattern_matcher.hpp"
#include "models/profile/profile_output_properties.hpp"
#include "monitor_specs.hpp"
#include "utils/string_list_edit.hpp"

ProfileEditorController::ProfileEditorController(BackendManager * backend_manager, QObject * parent)
    : QObject(parent),
      backend_manager_(backend_manager) {
    refresh_connected_outputs();
    refresh_daemon_status();

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

void ProfileEditorController::save_profile() {
    if (selected_profile_id_.isEmpty()) {
        return;
    }

    try {
        profile::ProfileDefinition profile = current_profile();
        profile_backend().add_profile(working_config_, std::move(profile));
        profile_backend().save_config(working_config_);
        saved_config_ = working_config_;
        set_dirty(false);
        emit profile_ids_changed();
        refresh_daemon_status();
        daemon_status_text_ += QStringLiteral(" · saved");
        emit daemon_status_changed();
    } catch (const std::exception & e) {
        daemon_status_text_ = QString::fromStdString(std::string("Save failed: ") + e.what());
        emit daemon_status_changed();
    }
}

void ProfileEditorController::discard_changes() {
    working_config_ = saved_config_;
    reload_current_profile_into_editor();
    set_dirty(false);
    emit profile_ids_changed();
    emit on_no_match_exec_commands_changed();
    emit discard_requested();
}

void ProfileEditorController::reload_config_from_disk() {
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
    emit capabilities_changed();
    reload_config_from_disk();
    refresh_connected_outputs();
    refresh_daemon_status();
}

void ProfileEditorController::switch_profile(bool force) {
    if (selected_profile_id_.isEmpty()) {
        return;
    }

    try {
        profile_backend().switch_profile(selected_profile_id_.toStdString(), force);
        refresh_daemon_status();
    } catch (const std::exception & e) {
        daemon_status_text_ = QString::fromStdString(std::string("Switch failed: ") + e.what());
        emit daemon_status_changed();
    }
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

    profile::ProfileDefinition profile =
        profile_backend().create_profile_from_live(profile_id.toStdString());
    profile_backend().add_profile(working_config_, std::move(profile));
    selected_profile_id_ = profile_id;
    reload_current_profile_into_editor();
    set_dirty(true);
    emit profile_ids_changed();
    emit selected_profile_id_changed();
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
    if (profile_id.isEmpty()) {
        return;
    }

    profile::ProfileDocument updated_config = working_config_;
    try {
        profile_backend().delete_profile(updated_config, profile_id.toStdString());
        profile_backend().save_config(updated_config);
        working_config_ = std::move(updated_config);
        saved_config_ = working_config_;

        if (selected_profile_id_ == profile_id) {
            selected_profile_id_.clear();
            clear_outputs();
            if (!working_config_.profiles.empty()) {
                selected_profile_id_ = QString::fromStdString(working_config_.profiles.front().id);
                rebuild_outputs_from_profile(working_config_.profiles.front());
            }
            emit selected_profile_id_changed();
        }

        set_dirty(false);
        emit profile_ids_changed();
        refresh_daemon_status();
    } catch (const std::exception & e) {
        daemon_status_text_ = QString::fromStdString(std::string("Delete failed: ") + e.what());
        emit daemon_status_changed();
    }
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
    try {
        const profile::ProfileServiceStatus status = profile_backend().service_status();
        daemon_running_ = status.service_running;
        active_profile_id_ = QString::fromStdString(status.active_profile);
        if (daemon_running_) {
            daemon_status_text_ =
                QStringLiteral("Daemon running · active: %1").arg(active_profile_id_);
        } else {
            daemon_status_text_ = QStringLiteral("Daemon not running");
        }
    } catch (const std::exception & e) {
        daemon_running_ = false;
        daemon_status_text_ =
            QString::fromStdString(std::string("Daemon unavailable: ") + e.what());
    }

    emit daemon_status_changed();
    emit active_profile_id_changed();
}

void ProfileEditorController::refresh_connected_outputs() {
    connected_output_infos_ = profile_backend().get_connected_outputs();
    connected_outputs_.clear();
    for (const profile::ConnectedOutput & output : connected_output_infos_) {
        connected_outputs_.append(QString::fromStdString(output.display_label()));
    }

    for (ProfileOutputProperties * output_model : output_models_) {
        output_model->refresh_match_preview();
        output_model->refresh_live_position();
    }

    update_match_warning();
    emit connected_outputs_changed();
}

auto ProfileEditorController::get_live_position_for_output(const QString & output_pattern) const
    -> QPoint {
    const std::vector<ConnectedOutputInfo> connected =
        AutoWlrRandrPatternMatcher::get_connected_outputs();
    const std::optional<ConnectedOutputInfo> matched =
        AutoWlrRandrPatternMatcher::find_matching_output(output_pattern.toStdString(), connected);
    if (!matched.has_value()) {
        return { 0, 0 };
    }

    const auto monitors = get_monitor_specs_list();
    for (const MonitorSpecs & monitor : monitors) {
        if (monitor.get_name() != matched->name) {
            continue;
        }

        if (!monitor.is_enabled() || !monitor.get_enabled_monitor_settings().has_value()) {
            return { 0, 0 };
        }

        const Position & position = monitor.get_enabled_monitor_settings()->get_position();
        return { position.x, position.y };
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
    if (dirty_ == dirty) {
        return;
    }
    dirty_ = dirty;
    emit is_dirty_changed();
}
