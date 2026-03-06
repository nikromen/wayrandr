#pragma once

#include <qlist.h>
#include <qtmetamacros.h>

#include <QHash>
#include <QObject>
#include <QPoint>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <memory>
#include <vector>

#include "../backend.hpp"
#include "models/profile/profile_editor_controller.hpp"
#include "monitor_specs.hpp"

class MonitorBlock;

class MainWindow : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool operationBusy READ is_busy NOTIFY confirmation_pending_changed)
    Q_PROPERTY(QList<QObject *> monitors READ get_monitors NOTIFY monitors_changed)
    Q_PROPERTY(
        int backendMode READ get_backend_mode WRITE set_backend_mode NOTIFY backend_mode_changed
    )
    Q_PROPERTY(ProfileEditorController * profileEditor READ get_profile_editor CONSTANT)
    Q_PROPERTY(
        bool confirmationPending READ is_confirmation_pending NOTIFY confirmation_pending_changed
    )
    Q_PROPERTY(
        int confirmationSecondsLeft READ get_confirmation_seconds_left NOTIFY
            confirmation_seconds_left_changed
    )
    Q_PROPERTY(
        bool confirmationAllowed READ is_confirmation_allowed NOTIFY confirmation_pending_changed
    )
    Q_PROPERTY(QString applyError READ get_apply_error NOTIFY apply_error_changed)
    Q_PROPERTY(bool kanshiAvailable READ is_kanshi_available CONSTANT)
    Q_PROPERTY(bool autoWlrRandrAvailable READ is_auto_wlr_randr_available CONSTANT)

public:
    explicit MainWindow(QObject * parent = nullptr);
    ~MainWindow() override;

    [[nodiscard]] bool is_busy() const {
        return backend_manager_->is_operation_busy() || !initialized_;
    }

    [[nodiscard]] auto get_monitors() const -> QList<QObject *>;
    [[nodiscard]] auto get_backend_mode() const -> int;
    [[nodiscard]] auto get_profile_editor() const -> ProfileEditorController *;
    [[nodiscard]] auto is_confirmation_pending() const -> bool;
    [[nodiscard]] auto get_confirmation_seconds_left() const -> int;
    [[nodiscard]] auto is_kanshi_available() const -> bool;
    [[nodiscard]] auto is_auto_wlr_randr_available() const -> bool;

    Q_INVOKABLE QObject * get_monitor_block(const QString & monitor_name);  // NOLINT

    Q_INVOKABLE void apply();
    Q_INVOKABLE static void save();
    Q_INVOKABLE void set_backend(int backend_type);
    Q_INVOKABLE void set_backend_mode(int backend_mode);
    Q_INVOKABLE [[nodiscard]] bool can_switch_backend(int backend_mode) const;
    Q_INVOKABLE void confirm_apply();
    Q_INVOKABLE void cancel_apply();
    Q_INVOKABLE bool prepare_close();
    [[nodiscard]] auto is_confirmation_allowed() const -> bool;
    [[nodiscard]] auto get_apply_error() const -> QString;

    Q_INVOKABLE QPoint snap_position(
        QObject * monitor,
        int x,
        int y,
        int snap_threshold,
        int canvas_width,
        int canvas_height,
        float display_scale
    ) const;                                                     // NOLINT
    Q_INVOKABLE int get_monitor_index(QObject * monitor) const;  // NOLINT
    Q_INVOKABLE void reset_canvas_layout(int canvas_width, float display_scale);

signals:
    void monitors_changed();
    void close_ready();
    void switch_to_monitor_tab(int index);
    void backend_mode_changed();
    void apply_error_changed();
    void confirmation_pending_changed();
    void confirmation_seconds_left_changed();

private slots:
    void on_confirmation_timeout();
    void on_countdown_tick();

private:  // NOLINT: Qt moc requires this to end the slots section.
    void finish_close();
    void initialize_monitors(std::vector<MonitorSpecs> specs);
    void reload_monitors(const std::vector<MonitorSpecs> & fresh_specs);
    void sync_confirmation_state();
    void set_apply_error(const QString & error);
    void stop_confirmation_timers();
    void reload_monitors_safely();
    void set_confirmation_pending(bool pending);
    void set_confirmation_seconds_left(int seconds);
    void set_backend_mode_internal(int backend_mode);

    static constexpr int K_CONFIRMATION_TIMEOUT_SECONDS = 15;
    static constexpr int K_BACKEND_MODE_WLR_RANDR = 0;
    static constexpr int K_BACKEND_MODE_KANSHI = 1;
    static constexpr int K_BACKEND_MODE_AUTO_WLR_RANDR = 2;

    std::vector<MonitorSpecs> monitors_;
    QList<QObject *> monitors_models_;
    QHash<QString, MonitorBlock *> monitor_blocks_;
    QTimer * confirmation_timer_;
    QTimer * countdown_timer_;
    QString apply_error_;
    bool close_requested_ = false;
    bool cancel_requested_ = false;
    bool initialized_ = false;
    bool confirmation_pending_ = false;
    int confirmation_seconds_left_ = 0;
    int backend_mode_ = K_BACKEND_MODE_WLR_RANDR;
    bool kanshi_available_ = false;
    bool auto_wlr_randr_available_ = false;
    std::shared_ptr<BackendManager> backend_manager_;
    std::unique_ptr<ProfileEditorController> profile_editor_;
};
