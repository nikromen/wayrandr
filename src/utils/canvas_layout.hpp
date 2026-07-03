#pragma once

#include <QPoint>
#include <cstddef>
#include <vector>

namespace canvas_layout {

inline constexpr int K_DEFAULT_SNAP_THRESHOLD = 200;

struct LayoutSize {
    int width = 0;
    int height = 0;
};

struct CanvasRect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    bool active = false;
};

struct ResettableItem {
    int layout_width = 0;
    int position_x = 0;
    int position_y = 0;
    bool active = false;
};

// Preview only: nonfinite/nonpositive scale, negative pixels or rounded dimensions
// outside int range produce 0x0. Stored settings are never normalized by this calculation.
[[nodiscard]] auto compute_layout_size(int pixel_width, int pixel_height, float scale, bool rotated)
    -> LayoutSize;

// Active rectangles snap into nonnegative int coordinates; negative extents count as zero.
// Canvas bounds apply only with positive dimensions and a finite positive display scale.
// Inactive rectangles retain their original coordinates, including negative ones.
[[nodiscard]] auto snap_position(
    const CanvasRect & current,
    const std::vector<CanvasRect> & others,
    int snap_threshold,
    int canvas_width,
    int canvas_height,
    float display_scale
) -> QPoint;

// Active positions start at zero and saturate at INT_MAX; negative widths count as zero.
// Saturation may overlap outputs when their complete layout cannot fit in int coordinates.
void reset_horizontal_layout(std::vector<ResettableItem> & items);

[[nodiscard]] auto snap_from_rects(
    const CanvasRect & current,
    const std::vector<CanvasRect> & all_rects,
    size_t current_index,
    int x,
    int y,
    int snap_threshold,
    int canvas_width,
    int canvas_height,
    float display_scale
) -> QPoint;

}  // namespace canvas_layout
