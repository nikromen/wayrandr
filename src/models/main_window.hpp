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

public:
    explicit MainWindow(QObject * parent = nullptr);

    [[nodiscard]] auto get_monitors() const -> QList<QObject *>;
    Q_INVOKABLE QObject * get_monitor_block(const QString & monitor_name);  // NOLINT

    Q_INVOKABLE static void apply();
    Q_INVOKABLE static void save();
    Q_INVOKABLE void set_backend(int backend_type);
    Q_INVOKABLE void confirm_apply();
    Q_INVOKABLE void cancel_apply();

    Q_INVOKABLE QPoint
    snap_position(QObject * monitor, int x, int y, int snap_threshold) const;  // NOLINT
    Q_INVOKABLE int get_monitor_index(QObject * monitor) const;                // NOLINT

signals:
    void switch_to_monitor_tab(int index);

private slots:
    void on_confirmation_timeout();

private:  // NOLINT
    std::vector<MonitorSpecs> monitors_;
    QList<QObject *> monitors_models_;
    QHash<QString, MonitorBlock *> monitor_blocks_;
    QTimer * confirmation_timer_;
    std::unique_ptr<BackendManager> backend_manager_;
};
