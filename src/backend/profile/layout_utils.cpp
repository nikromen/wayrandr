#include "backend/profile/layout_utils.hpp"

#include "monitor_specs.hpp"
#include "utils/canvas_layout.hpp"

namespace profile::layout_utils {

auto parse_mode_string(const std::string & mode) -> std::optional<std::pair<int, int>> {
    const auto at_pos = mode.find('@');
    const std::string resolution = at_pos == std::string::npos ? mode : mode.substr(0, at_pos);
    const auto x_pos = resolution.find('x');
    if (x_pos == std::string::npos) {
        return std::nullopt;
    }

    try {
        const int width = std::stoi(resolution.substr(0, x_pos));
        const int height = std::stoi(resolution.substr(x_pos + 1));
        return std::make_pair(width, height);
    } catch (...) {
        return std::nullopt;
    }
}

auto is_transform_rotated(const std::string & transform) -> bool {
    return transform_utils::is_rotated(transform);
}

auto layout_width(const ProfileOutputDefinition & output) -> int {
    const auto parsed =
        output.mode.has_value() ? parse_mode_string(output.mode.value()) : std::nullopt;
    const int pixel_width = parsed.has_value() ? parsed->first : 1920;
    const int pixel_height = parsed.has_value() ? parsed->second : 1080;
    const float scale = output.scale.value_or(1.0F);
    const std::string transform = output.transform.value_or("normal");
    return canvas_layout::compute_layout_size(
               pixel_width, pixel_height, scale, is_transform_rotated(transform)
    )
        .width;
}

auto layout_height(const ProfileOutputDefinition & output) -> int {
    const auto parsed =
        output.mode.has_value() ? parse_mode_string(output.mode.value()) : std::nullopt;
    const int pixel_width = parsed.has_value() ? parsed->first : 1920;
    const int pixel_height = parsed.has_value() ? parsed->second : 1080;
    const float scale = output.scale.value_or(1.0F);
    const std::string transform = output.transform.value_or("normal");
    return canvas_layout::compute_layout_size(
               pixel_width, pixel_height, scale, is_transform_rotated(transform)
    )
        .height;
}

auto parse_position(const std::optional<std::string> & pos) -> std::optional<std::pair<int, int>> {
    if (!pos.has_value() || pos->empty()) {
        return std::nullopt;
    }

    const auto comma_pos = pos->find(',');
    if (comma_pos == std::string::npos) {
        return std::nullopt;
    }

    try {
        const int x = std::stoi(pos->substr(0, comma_pos));
        const int y = std::stoi(pos->substr(comma_pos + 1));
        return std::make_pair(x, y);
    } catch (...) {
        return std::nullopt;
    }
}

}  // namespace profile::layout_utils
