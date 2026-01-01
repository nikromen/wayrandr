#pragma once

#include <qcontainerfwd.h>
#include <qtmetamacros.h>

#include <QObject>
#include <QString>
#include <QStringList>

#include "backend/profile/types.hpp"
#include "utils/canvas_drag.hpp"

class ProfileEditorController;

class ProfileOutputProperties : public QObject {
    Q_OBJECT
    Q_PROPERTY(
        QString outputPattern READ get_output_pattern WRITE set_output_pattern NOTIFY
            output_pattern_changed
    )
    Q_PROPERTY(bool enabled READ is_enabled WRITE set_enabled NOTIFY enabled_changed)
    Q_PROPERTY(QString mode READ get_mode WRITE set_mode NOTIFY mode_changed)
    Q_PROPERTY(bool preferred READ is_preferred WRITE set_preferred NOTIFY preferred_changed)
    Q_PROPERTY(int positionX READ get_position_x WRITE set_position_x NOTIFY position_x_changed)
    Q_PROPERTY(int positionY READ get_position_y WRITE set_position_y NOTIFY position_y_changed)
    Q_PROPERTY(float scale READ get_scale WRITE set_scale NOTIFY scale_changed)
    Q_PROPERTY(QString transform READ get_transform WRITE set_transform NOTIFY transform_changed)
    Q_PROPERTY(
        bool adaptiveSync READ is_adaptive_sync WRITE set_adaptive_sync NOTIFY adaptive_sync_changed
    )
    Q_PROPERTY(int layoutWidth READ get_layout_width NOTIFY layout_dimensions_changed)
    Q_PROPERTY(int layoutHeight READ get_layout_height NOTIFY layout_dimensions_changed)
    Q_PROPERTY(QString matchPreview READ get_match_preview NOTIFY match_preview_changed)
    Q_PROPERTY(QStringList transformList READ get_transform_list CONSTANT)

public:
    explicit ProfileOutputProperties(ProfileEditorController * editor, QObject * parent = nullptr);

    void load_from_output(const profile::ProfileOutputDefinition & output);
    [[nodiscard]] auto to_output() const -> profile::ProfileOutputDefinition;
    void refresh_match_preview();
    void refresh_live_position();

    [[nodiscard]] auto get_output_pattern() const -> QString;
    [[nodiscard]] auto is_enabled() const -> bool;
    void set_enabled(bool enabled);
    [[nodiscard]] auto get_mode() const -> QString;
    [[nodiscard]] auto is_preferred() const -> bool;
    [[nodiscard]] auto get_position_x() const -> int;
    [[nodiscard]] auto get_position_y() const -> int;
    [[nodiscard]] auto get_scale() const -> float;
    [[nodiscard]] auto get_transform() const -> QString;
    [[nodiscard]] auto is_adaptive_sync() const -> bool;
    void set_adaptive_sync(bool adaptive_sync);
    [[nodiscard]] auto get_layout_width() const -> int;
    [[nodiscard]] auto get_layout_height() const -> int;
    [[nodiscard]] auto get_match_preview() const -> QString;
    static auto get_transform_list() -> QStringList;

    void set_output_pattern(const QString & pattern);
    void set_mode(const QString & mode);
    void set_preferred(bool preferred);
    Q_INVOKABLE void set_position_x(int x);
    Q_INVOKABLE void set_position_y(int y);
    void set_scale(float scale);
    void set_transform(const QString & transform);

    Q_INVOKABLE void start_drag(int mouse_x, int mouse_y);
    Q_INVOKABLE void update_drag(
        int mouse_x,
        int mouse_y,
        float display_scale,
        QObject * main_window,
        int canvas_width,
        int canvas_height
    );

signals:
    void output_pattern_changed();
    void enabled_changed();
    void mode_changed();
    void preferred_changed();
    void position_x_changed();
    void position_y_changed();
    void scale_changed();
    void transform_changed();
    void adaptive_sync_changed();
    void layout_dimensions_changed();
    void match_preview_changed();
    void setting_changed();

private:
    [[nodiscard]] auto has_explicit_position() const -> bool;
    void notify_layout_changed();
    void mark_dirty();

    ProfileEditorController * editor_;
    profile::ProfileOutputDefinition output_;
    bool enabled_ = true;
    bool adaptive_sync_ = false;
    QString match_preview_;
    canvas_drag::DragState drag_state_;
};
