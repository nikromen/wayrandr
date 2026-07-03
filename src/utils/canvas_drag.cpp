#include "utils/canvas_drag.hpp"

#include <QMetaObject>
#include <QPoint>
#include <algorithm>
#include <cmath>
#include <limits>

namespace {
auto drag_coordinate(int mouse, int start_mouse, int start_pos, float display_scale) -> int {
    const double delta = std::trunc((static_cast<double>(mouse) - start_mouse) / display_scale);
    return static_cast<int>(std::clamp(
        start_pos + delta,
        static_cast<double>(std::numeric_limits<int>::min()),
        static_cast<double>(std::numeric_limits<int>::max())
    ));
}
}  // namespace

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
    if (main_window == nullptr || item == nullptr || !std::isfinite(display_scale) ||
        display_scale <= 0.0F) {
        return;
    }

    const int new_x =
        drag_coordinate(mouse_x, state.start_mouse_x, state.start_pos_x, display_scale);
    const int new_y =
        drag_coordinate(mouse_y, state.start_mouse_y, state.start_pos_y, display_scale);

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
