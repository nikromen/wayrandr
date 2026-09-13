#include "utils/canvas_layout.hpp"

#include <algorithm>
#include <cmath>

namespace canvas_layout {

auto compute_layout_size(int pixel_width, int pixel_height, float scale, bool rotated)
    -> LayoutSize {
    if (scale <= 0.0F) {
        return { pixel_width, pixel_height };
    }

    const int logical_width = static_cast<int>(std::round(pixel_width / scale));
    const int logical_height = static_cast<int>(std::round(pixel_height / scale));
    if (rotated) {
        return { logical_height, logical_width };
    }
    return { logical_width, logical_height };
}

auto snap_position(
    const CanvasRect & current,
    const std::vector<CanvasRect> & others,
    int snap_threshold,
    int canvas_width,
    int canvas_height,
    float display_scale
) -> QPoint {
    if (!current.active) {
        return { current.x, current.y };
    }

    int snapped_x = std::max(0, current.x);
    int snapped_y = std::max(0, current.y);

    const int layout_width = current.width;
    const int layout_height = current.height;

    for (const CanvasRect & other : others) {
        if (!other.active) {
            continue;
        }

        const int other_left = other.x;
        const int other_right = other.x + other.width;
        const int other_top = other.y;
        const int other_bottom = other.y + other.height;

        int this_left = snapped_x;
        int this_right = snapped_x + layout_width;
        int this_top = snapped_y;
        int this_bottom = snapped_y + layout_height;

        if (std::abs(this_left - other_right) < snap_threshold) {
            snapped_x = other_right;
        } else if (std::abs(this_right - other_left) < snap_threshold) {
            snapped_x = other_left - layout_width;
        } else if (std::abs(this_left - other_left) < snap_threshold) {
            snapped_x = other_left;
        } else if (std::abs(this_right - other_right) < snap_threshold) {
            snapped_x = other_right - layout_width;
        }

        this_left = snapped_x;
        this_right = snapped_x + layout_width;
        this_top = snapped_y;
        this_bottom = snapped_y + layout_height;

        if (std::abs(this_top - other_bottom) < snap_threshold) {
            snapped_y = other_bottom;
        } else if (std::abs(this_bottom - other_top) < snap_threshold) {
            snapped_y = other_top - layout_height;
        } else if (std::abs(this_top - other_top) < snap_threshold) {
            snapped_y = other_top;
        } else if (std::abs(this_bottom - other_bottom) < snap_threshold) {
            snapped_y = other_bottom - layout_height;
        }
    }

    snapped_x = std::max(0, snapped_x);
    snapped_y = std::max(0, snapped_y);

    if (canvas_width > 0 && canvas_height > 0 && display_scale > 0.0F) {
        const int max_x =
            std::max(0, static_cast<int>(canvas_width / display_scale) - layout_width);
        const int max_y =
            std::max(0, static_cast<int>(canvas_height / display_scale) - layout_height);
        snapped_x = std::min(snapped_x, max_x);
        snapped_y = std::min(snapped_y, max_y);
    }

    return { snapped_x, snapped_y };
}

auto snap_from_rects(
    const CanvasRect & current,
    const std::vector<CanvasRect> & all_rects,
    size_t current_index,
    int x,
    int y,
    int snap_threshold,
    int canvas_width,
    int canvas_height,
    float display_scale
) -> QPoint {
    CanvasRect moving_rect = current;
    moving_rect.x = x;
    moving_rect.y = y;

    std::vector<CanvasRect> other_rects;
    for (size_t index = 0; index < all_rects.size(); ++index) {
        if (index == current_index) {
            continue;
        }
        other_rects.push_back(all_rects[index]);
    }

    return snap_position(
        moving_rect, other_rects, snap_threshold, canvas_width, canvas_height, display_scale
    );
}

void reset_horizontal_layout(std::vector<ResettableItem> & items) {
    int x = 0;
    const int y = 0;

    for (ResettableItem & item : items) {
        if (!item.active) {
            continue;
        }

        item.position_x = x;
        item.position_y = y;
        x += item.layout_width;
    }
}

}  // namespace canvas_layout
