#include "utils/canvas_layout.hpp"

#include <qpoint.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace canvas_layout {

auto compute_layout_size(int pixel_width, int pixel_height, float scale, bool rotated)
    -> LayoutSize {
    if (!std::isfinite(scale) || scale <= 0.0F || pixel_width < 0 || pixel_height < 0) {
        return {};
    }

    const double width = std::round(pixel_width / static_cast<double>(scale));
    const double height = std::round(pixel_height / static_cast<double>(scale));
    if (!std::isfinite(width) || !std::isfinite(height) ||
        width > std::numeric_limits<int>::max() || height > std::numeric_limits<int>::max()) {
        return {};
    }
    const int logical_width = static_cast<int>(width);
    const int logical_height = static_cast<int>(height);
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

    int64_t snapped_x = std::max(0, current.x);
    int64_t snapped_y = std::max(0, current.y);

    const int64_t layout_width = std::max(0, current.width);
    const int64_t layout_height = std::max(0, current.height);

    for (const CanvasRect & other : others) {
        if (!other.active) {
            continue;
        }

        const int64_t other_left = other.x;
        const int64_t other_right = other_left + std::max(0, other.width);
        const int64_t other_top = other.y;
        const int64_t other_bottom = other_top + std::max(0, other.height);

        const int64_t this_left = snapped_x;
        const int64_t this_right = snapped_x + layout_width;
        const int64_t this_top = snapped_y;
        const int64_t this_bottom = snapped_y + layout_height;

        if (std::abs(this_left - other_right) < snap_threshold) {
            snapped_x = other_right;
        } else if (std::abs(this_right - other_left) < snap_threshold) {
            snapped_x = other_left - layout_width;
        } else if (std::abs(this_left - other_left) < snap_threshold) {
            snapped_x = other_left;
        } else if (std::abs(this_right - other_right) < snap_threshold) {
            snapped_x = other_right - layout_width;
        }

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

    snapped_x = std::clamp<int64_t>(snapped_x, 0, std::numeric_limits<int>::max());
    snapped_y = std::clamp<int64_t>(snapped_y, 0, std::numeric_limits<int>::max());

    if (canvas_width > 0 && canvas_height > 0 && std::isfinite(display_scale) &&
        display_scale > 0.0F) {
        const auto max_x = static_cast<int64_t>(std::clamp(
            std::trunc(canvas_width / static_cast<double>(display_scale)) - layout_width,
            0.0,
            static_cast<double>(std::numeric_limits<int>::max())
        ));
        const auto max_y = static_cast<int64_t>(std::clamp(
            std::trunc(canvas_height / static_cast<double>(display_scale)) - layout_height,
            0.0,
            static_cast<double>(std::numeric_limits<int>::max())
        ));
        snapped_x = std::min(snapped_x, max_x);
        snapped_y = std::min(snapped_y, max_y);
    }

    return { static_cast<int>(snapped_x), static_cast<int>(snapped_y) };
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
        x += std::clamp(item.layout_width, 0, std::numeric_limits<int>::max() - x);
    }
}

}  // namespace canvas_layout
