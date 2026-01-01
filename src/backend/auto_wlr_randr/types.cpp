#include "backend/auto_wlr_randr/types.hpp"

#include <utility>

#include "backend/auto_wlr_randr/conversions.hpp"
#include "backend/profile/layout_utils.hpp"

auto ConnectedOutputInfo::build_identifier() const -> std::optional<std::string> {
    return auto_wlr_randr_conversions::to_connected_output(*this).build_identifier();
}

auto ConnectedOutputInfo::display_label() const -> std::string {
    return auto_wlr_randr_conversions::to_connected_output(*this).display_label();
}

namespace profile_mode_utils {

auto parse_mode_string(const std::string & mode) -> std::optional<std::pair<int, int>> {
    return profile::layout_utils::parse_mode_string(mode);
}

auto is_transform_rotated(const std::string & transform) -> bool {
    return profile::layout_utils::is_transform_rotated(transform);
}

auto layout_width(const ProfileOutputSetting & setting) -> int {
    return profile::layout_utils::layout_width(
        auto_wlr_randr_conversions::to_profile_output(setting)
    );
}

auto layout_height(const ProfileOutputSetting & setting) -> int {
    return profile::layout_utils::layout_height(
        auto_wlr_randr_conversions::to_profile_output(setting)
    );
}

auto parse_position(const std::optional<std::string> & pos) -> std::optional<std::pair<int, int>> {
    return profile::layout_utils::parse_position(pos);
}

}  // namespace profile_mode_utils
