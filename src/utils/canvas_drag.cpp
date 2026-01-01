#include "utils/canvas_drag.hpp"

#include <QMetaObject>
#include <QPoint>

namespace canvas_drag {

void start(DragState & state, int mouse_x, int mouse_y, int pos_x, int pos_y) {
    state.start_mouse_x = mouse_x;
    state.start_mouse_y = mouse_y;
    state.start_pos_x = pos_x;
    state.start_pos_y = pos_y;
}

void update_with_snap(
    const DragState & state,
    int mouse_x,
    int mouse_y,
    float display_scale,
    QObject * item,
    QObject * main_window,
    int canvas_width,
    int canvas_height,
    int snap_threshold
) {
    if (main_window == nullptr || item == nullptr || display_scale <= 0.0F) {
        return;
    }

    const int delta_x = static_cast<int>((mouse_x - state.start_mouse_x) / display_scale);
    const int delta_y = static_cast<int>((mouse_y - state.start_mouse_y) / display_scale);
    const int new_x = state.start_pos_x + delta_x;
    const int new_y = state.start_pos_y + delta_y;

    QPoint snapped(new_x, new_y);
    QMetaObject::invokeMethod(
        main_window,
        "snap_position",
        Q_RETURN_ARG(QPoint, snapped),
        Q_ARG(QObject *, item),
        Q_ARG(int, new_x),
        Q_ARG(int, new_y),
        Q_ARG(int, snap_threshold),
        Q_ARG(int, canvas_width),
        Q_ARG(int, canvas_height),
        Q_ARG(float, display_scale)
    );

    QMetaObject::invokeMethod(item, "set_position_x", Q_ARG(int, snapped.x()));
    QMetaObject::invokeMethod(item, "set_position_y", Q_ARG(int, snapped.y()));
}

}  // namespace canvas_drag
