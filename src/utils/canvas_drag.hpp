#pragma once

#include <QObject>

namespace canvas_drag {

struct DragState {
    int start_mouse_x = 0;
    int start_mouse_y = 0;
    int start_pos_x = 0;
    int start_pos_y = 0;
};

void start(DragState & state, int mouse_x, int mouse_y, int pos_x, int pos_y);

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
);

}  // namespace canvas_drag
