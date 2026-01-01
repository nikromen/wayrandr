#pragma once

#include <optional>
#include <string>
#include <utility>

#include "backend/profile/types.hpp"

namespace profile::layout_utils {

[[nodiscard]] auto parse_mode_string(const std::string & mode)
    -> std::optional<std::pair<int, int>>;
[[nodiscard]] auto is_transform_rotated(const std::string & transform) -> bool;
[[nodiscard]] auto layout_width(const ProfileOutputDefinition & output) -> int;
[[nodiscard]] auto layout_height(const ProfileOutputDefinition & output) -> int;
[[nodiscard]] auto parse_position(const std::optional<std::string> & pos)
    -> std::optional<std::pair<int, int>>;

}  // namespace profile::layout_utils
