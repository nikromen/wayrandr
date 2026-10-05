#pragma once

#include <qcontainerfwd.h>
#include <qlist.h>
#include <qtmetamacros.h>

#include <QObject>
#include <QPoint>
#include <QString>
#include <QStringList>
#include <functional>
#include <string>
#include <vector>

#include "../../backend.hpp"
#include "backend/profile/editor_backend.hpp"
#include "backend/profile/types.hpp"
#include "models/profile/profile_output_properties.hpp"
#include "monitor_specs.hpp"
#include "utils/helpers.hpp"

class ProfileEditorController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool operationBusy READ is_operation_busy NOTIFY operation_busy_changed)
    Q_PROPERTY(QStringList profileIds READ get_profile_ids NOTIFY profile_ids_changed)
    Q_PROPERTY(
        QString selectedProfileId READ get_selected_profile_id WRITE set_selected_profile_id NOTIFY
            selected_profile_id_changed
    )
    Q_PROPERTY(
        bool configurationLoaded READ is_configuration_loaded NOTIFY configuration_loaded_changed
    )
    Q_PROPERTY(QString loadError READ get_load_error NOTIFY load_error_changed)
    Q_PROPERTY(QString configurationNotice READ get_configuration_notice NOTIFY profile_ids_changed)
    Q_PROPERTY(bool isDirty READ is_dirty NOTIFY is_dirty_changed)
    Q_PROPERTY(QString activeProfileId READ get_active_profile_id NOTIFY active_profile_id_changed)
    Q_PROPERTY(bool daemonRunning READ is_daemon_running NOTIFY daemon_status_changed)
    Q_PROPERTY(QString matchWarning READ get_match_warning NOTIFY match_warning_changed)
    Q_PROPERTY(QString daemonStatusText READ get_daemon_status_text NOTIFY daemon_status_changed)
    Q_PROPERTY(QString saveStatusText READ get_save_status_text NOTIFY save_status_changed)
    Q_PROPERTY(QList<QObject *> outputs READ get_outputs NOTIFY outputs_changed)
    Q_PROPERTY(QStringList execCommands READ get_exec_commands NOTIFY exec_commands_changed)
    Q_PROPERTY(
        QStringList onNoMatchExecCommands READ get_on_no_match_exec_commands NOTIFY
            on_no_match_exec_commands_changed
    )
    Q_PROPERTY(
        QStringList connectedOutputs READ get_connected_outputs NOTIFY connected_outputs_changed
    )
    Q_PROPERTY(
        bool supportsPatternMatching READ supports_pattern_matching NOTIFY capabilities_changed
    )
    Q_PROPERTY(
        bool supportsOutputMatchPreview READ supports_output_match_preview NOTIFY
            capabilities_changed
    )
    Q_PROPERTY(
        bool supportsServiceIntegration READ supports_service_integration NOTIFY
            capabilities_changed
    )
    Q_PROPERTY(bool supportsForceSwitch READ supports_force_switch NOTIFY capabilities_changed)
    Q_PROPERTY(
        bool supportsOnNoMatchExec READ supports_on_no_match_exec NOTIFY capabilities_changed
    )
    Q_PROPERTY(bool supportsPreferredMode READ supports_preferred_mode NOTIFY capabilities_changed)
    Q_PROPERTY(bool supportsAdaptiveSync READ supports_adaptive_sync NOTIFY capabilities_changed)

public:
    explicit ProfileEditorController(BackendManager * backend_manager, QObject * parent = nullptr);

    [[nodiscard]] bool is_operation_busy() const { return backend_manager_->is_operation_busy(); }

    [[nodiscard]] auto get_profile_ids() const -> QStringList;
    [[nodiscard]] auto get_selected_profile_id() const -> QString;

    [[nodiscard]] auto is_configuration_loaded() const -> bool { return configuration_loaded_; }

    [[nodiscard]] auto get_load_error() const -> QString { return load_error_; }

    [[nodiscard]] auto get_configuration_notice() const -> QString;
    [[nodiscard]] auto is_dirty() const -> bool;
    [[nodiscard]] auto get_active_profile_id() const -> QString;
    [[nodiscard]] auto is_daemon_running() const -> bool;
    [[nodiscard]] auto get_match_warning() const -> QString;
    [[nodiscard]] auto get_daemon_status_text() const -> QString;
    [[nodiscard]] auto get_save_status_text() const -> QString;
    [[nodiscard]] auto get_outputs() const -> QList<QObject *>;
    [[nodiscard]] auto get_exec_commands() const -> QStringList;
    [[nodiscard]] auto get_on_no_match_exec_commands() const -> QStringList;
    [[nodiscard]] auto get_connected_outputs() const -> QStringList;
    [[nodiscard]] auto get_connected_output_infos() const
        -> const std::vector<profile::ConnectedOutput> &;

    [[nodiscard]] auto supports_pattern_matching() const -> bool;
    [[nodiscard]] auto supports_output_match_preview() const -> bool;
    [[nodiscard]] auto supports_service_integration() const -> bool;
    [[nodiscard]] auto supports_force_switch() const -> bool;
    [[nodiscard]] auto supports_on_no_match_exec() const -> bool;
    [[nodiscard]] auto supports_preferred_mode() const -> bool;
    [[nodiscard]] auto supports_adaptive_sync() const -> bool;

    void set_selected_profile_id(const QString & profile_id);

    Q_INVOKABLE bool select_profile(const QString & profile_id, bool force = false);
    Q_INVOKABLE void save_profile();
    Q_INVOKABLE void discard_changes();
    Q_INVOKABLE void switch_profile(bool force = false);
    Q_INVOKABLE void create_profile_from_live(const QString & profile_id);
    Q_INVOKABLE void duplicate_profile(const QString & source_id, const QString & new_id);
    Q_INVOKABLE void delete_profile(const QString & profile_id);
    Q_INVOKABLE void add_output();
    Q_INVOKABLE void remove_output(int index);
    Q_INVOKABLE void add_exec_command(const QString & command);
    Q_INVOKABLE void remove_exec_command(int index);
    Q_INVOKABLE void set_exec_command(int index, const QString & command);
    Q_INVOKABLE void add_on_no_match_exec_command(const QString & command);
    Q_INVOKABLE void remove_on_no_match_exec_command(int index);
    Q_INVOKABLE void set_on_no_match_exec_command(int index, const QString & command);
    Q_INVOKABLE void refresh_daemon_status();
    Q_INVOKABLE void refresh_connected_outputs();
    Q_INVOKABLE void reload_config_from_disk();
    Q_INVOKABLE void on_profile_backend_changed();
    Q_INVOKABLE int get_output_index(QObject * output) const;

    void mark_dirty();
    void reload_current_profile_into_editor();
    [[nodiscard]] auto get_output_match_preview(const std::string & output_selector) const
        -> QString;
    [[nodiscard]] auto get_live_position_for_output(const QString & output_pattern) const -> QPoint;

signals:
    void operation_busy_changed();
    void profile_ids_changed();
    void selected_profile_id_changed();
    void is_dirty_changed();
    void configuration_loaded_changed();
    void load_error_changed();
    void active_profile_id_changed();
    void daemon_status_changed();
    void save_status_changed();
    void match_warning_changed();
    void outputs_changed();
    void exec_commands_changed();
    void on_no_match_exec_commands_changed();
    void connected_outputs_changed();
    void capabilities_changed();
    void discard_requested();

private:
    [[nodiscard]] auto profile_backend() const -> const ProfileEditorBackend &;
    [[nodiscard]] auto profile_backend() -> ProfileEditorBackend &;
    [[nodiscard]] auto capabilities() const -> profile::ProfileEditorCapabilities;

    [[nodiscard]] auto find_profile(const QString & profile_id) -> profile::ProfileDefinition *;
    [[nodiscard]] auto find_profile(const QString & profile_id) const
        -> const profile::ProfileDefinition *;
    [[nodiscard]] auto profile_exists(const QString & profile_id) const -> bool;
    [[nodiscard]] auto ensure_selected_profile_exists() -> profile::ProfileDefinition *;

    bool submit(
        std::function<Completion()> work, std::function<void(const std::string &)> failure = {}
    );
    void report_process_error(const std::string & error);
    void report_save_result(const ProfileSaveResult & result);
    void report_save_error(const std::string & error);
    std::vector<MonitorSpecs> live_monitors_;
    unsigned revision_ = 0;
    void clear_outputs();
    void rebuild_outputs_from_profile(const profile::ProfileDefinition & profile);
    [[nodiscard]] auto current_profile() const -> profile::ProfileDefinition;
    void update_match_warning();
    void set_dirty(bool dirty);

    BackendManager * backend_manager_;
    profile::ProfileDocument working_config_;
    profile::ProfileDocument saved_config_;
    QList<ProfileOutputProperties *> output_models_;
    QString selected_profile_id_;
    QString active_profile_id_;
    bool daemon_running_ = false;
    QString daemon_status_text_;
    QString save_status_text_;
    QString match_warning_;
    QStringList connected_outputs_;
    std::vector<profile::ConnectedOutput> connected_output_infos_;
    bool dirty_ = false;
    bool configuration_loaded_ = false;
    QString load_error_;
};
