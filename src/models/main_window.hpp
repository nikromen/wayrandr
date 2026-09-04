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
#include "monitor_specs.hpp"

class MonitorBlock;

class MainWindow : public QObject {
    Q_OBJECT
    Q_PROPERTY(QList<QObject *> monitors READ get_monitors CONSTANT)
    Q_PROPERTY(bool confirmationPending READ is_confirmation_pending NOTIFY confirmation_pending_changed)
    Q_PROPERTY(
        int confirmationSecondsLeft READ get_confirmation_seconds_left NOTIFY
            confirmation_seconds_left_changed
    )

public:
    explicit MainWindow(QObject * parent = nullptr);

    [[nodiscard]] auto get_monitors() const -> QList<QObject *>;
    [[nodiscard]] auto is_confirmation_pending() const -> bool;
    [[nodiscard]] auto get_confirmation_seconds_left() const -> int;

    Q_INVOKABLE QObject * get_monitor_block(const QString & monitor_name);  // NOLINT

    Q_INVOKABLE void apply();
    Q_INVOKABLE void save();
    Q_INVOKABLE void set_backend(int backend_type);
    Q_INVOKABLE void confirm_apply();
    Q_INVOKABLE void cancel_apply();

    Q_INVOKABLE QPoint snap_position(
        QObject * monitor,
        int x,
        int y,
        int snap_threshold,
        int canvas_width,
        int canvas_height,
        float display_scale
    ) const;  // NOLINT
    Q_INVOKABLE int get_monitor_index(QObject * monitor) const;                // NOLINT
    Q_INVOKABLE void reset_canvas_layout(int canvas_width, float display_scale);

signals:
    void switch_to_monitor_tab(int index);
    void confirmation_pending_changed();
    void confirmation_seconds_left_changed();

private slots:
    void on_confirmation_timeout();
    void on_countdown_tick();

private:
    void reload_monitors();
    void set_confirmation_pending(bool pending);
    void set_confirmation_seconds_left(int seconds);

    static constexpr int kConfirmationTimeoutSeconds = 15;

    std::vector<MonitorSpecs> monitors_;
    QList<QObject *> monitors_models_;
    QHash<QString, MonitorBlock *> monitor_blocks_;
    QTimer * confirmation_timer_;
    QTimer * countdown_timer_;
    bool confirmation_pending_ = false;
    int confirmation_seconds_left_ = 0;
    std::unique_ptr<BackendManager> backend_manager_;
};
