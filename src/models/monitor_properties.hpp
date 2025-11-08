#pragma once

#include <qcontainerfwd.h>
#include <qtmetamacros.h>

#include <QObject>
#include <QString>
#include <QStringList>
#include <cstddef>

#include "monitor_specs.hpp"

class MonitorProperties : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool enabled READ is_enabled WRITE set_enabled NOTIFY enabled_changed)
    Q_PROPERTY(bool hasSettings READ has_settings NOTIFY enabled_changed)
    Q_PROPERTY(QString name READ get_name CONSTANT)
    Q_PROPERTY(QString description READ get_description CONSTANT)
    Q_PROPERTY(QStringList resolutions READ get_resolutions CONSTANT)  // modes

    // Settings properties - only valid when hasSettings is true
    Q_PROPERTY(
        bool adaptiveSync READ is_adaptive_sync WRITE set_adaptive_sync NOTIFY adaptive_sync_changed
    )
    Q_PROPERTY(
        size_t activeResolutionIndex READ get_active_resolution_index WRITE
            set_active_resolution_index NOTIFY active_resolution_index_changed
    )
    Q_PROPERTY(float scale READ get_scale WRITE set_scale NOTIFY scale_changed)
    Q_PROPERTY(int positionX READ get_position_x WRITE set_position_x NOTIFY position_x_changed)
    Q_PROPERTY(int positionY READ get_position_y WRITE set_position_y NOTIFY position_y_changed)
    Q_PROPERTY(int resolutionWidth READ get_resolution_width NOTIFY active_resolution_index_changed)
    Q_PROPERTY(
        int resolutionHeight READ get_resolution_height NOTIFY active_resolution_index_changed
    )
    Q_PROPERTY(QString transform READ get_transform WRITE set_transform NOTIFY transform_changed)
    Q_PROPERTY(QStringList transformList READ get_transform_list CONSTANT)

public:
    explicit MonitorProperties(MonitorSpecs * specs, QObject * parent = nullptr);

    // Getters
    [[nodiscard]] auto is_enabled() const -> bool;
    [[nodiscard]] auto has_settings() const -> bool;
    [[nodiscard]] auto get_name() const -> QString;
    [[nodiscard]] auto get_description() const -> QString;
    [[nodiscard]] auto get_resolutions() const -> QStringList;
    static auto get_transform_list() -> QStringList;

    // Settings getters - only valid when hasSettings is true
    [[nodiscard]] auto is_adaptive_sync() const -> bool;
    [[nodiscard]] auto get_active_resolution_index() const -> size_t;
    [[nodiscard]] auto get_scale() const -> float;
    [[nodiscard]] auto get_position_x() const -> int;
    [[nodiscard]] auto get_position_y() const -> int;
    [[nodiscard]] auto get_resolution_width() const -> int;
    [[nodiscard]] auto get_resolution_height() const -> int;
    [[nodiscard]] auto get_transform() const -> QString;

    // Setters
    void set_enabled(bool enabled);
    void set_adaptive_sync(bool adaptive_sync);
    void set_active_resolution_index(size_t index);
    void set_scale(float scale);
    void set_position_x(int x);
    void set_position_y(int y);
    void set_transform(const QString & transform);

    Q_INVOKABLE void start_drag(int mouse_x, int mouse_y);
    Q_INVOKABLE void update_drag(
        int mouse_x, int mouse_y, float display_scale, QObject * main_window
    );
    Q_INVOKABLE void activate_preferred_mode();

signals:
    void enabled_changed();
    void adaptive_sync_changed();
    void active_resolution_index_changed();
    void scale_changed();
    void position_x_changed();
    void position_y_changed();
    void transform_changed();

private:
    [[nodiscard]] auto get_resolution(bool width) const -> int;

    MonitorSpecs * monitor_specs;
    int drag_start_mouse_x_ = 0;
    int drag_start_mouse_y_ = 0;
    int drag_start_pos_x_ = 0;
    int drag_start_pos_y_ = 0;
    int current_drag_x_ = 0;
    int current_drag_y_ = 0;
};
