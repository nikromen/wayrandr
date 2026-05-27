#include "backend/kanshi/types.hpp"

#include <optional>
#include <string>
#include <utility>

#include "backend/kanshi/conversions.hpp"
#include "backend/profile/layout_utils.hpp"

namespace kanshi_profile_mode_utils {

auto parse_mode_string(const std::string & mode) -> std::optional<std::pair<int, int>> {
    return profile::layout_utils::parse_mode_string(mode);
}

auto is_transform_rotated(const std::string & transform) -> bool {
    return profile::layout_utils::is_transform_rotated(transform);
}

auto layout_width(const KanshiOutputSetting & setting) -> int {
    return profile::layout_utils::layout_width(kanshi_conversions::to_profile_output(setting));
}

auto layout_height(const KanshiOutputSetting & setting) -> int {
    return profile::layout_utils::layout_height(kanshi_conversions::to_profile_output(setting));
}

auto parse_position(const std::optional<std::string> & position)
    -> std::optional<std::pair<int, int>> {
    return profile::layout_utils::parse_position(position);
}

}  // namespace kanshi_profile_mode_utils
