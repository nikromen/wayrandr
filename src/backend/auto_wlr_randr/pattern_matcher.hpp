#pragma once

#include <optional>
#include <string>
#include <vector>

#include "backend/auto_wlr_randr/types.hpp"

class AutoWlrRandrPatternMatcher {
public:
    [[nodiscard]] static auto matches_pattern(
        const std::string & pattern, const ConnectedOutputInfo & output
    ) -> bool;

    [[nodiscard]] static auto find_matching_output(
        const std::string & pattern, const std::vector<ConnectedOutputInfo> & outputs
    ) -> std::optional<ConnectedOutputInfo>;

    [[nodiscard]] static auto would_profile_match(
        const AutoWlrRandrProfile & profile,
        const std::vector<ConnectedOutputInfo> & connected_outputs
    ) -> bool;

    [[nodiscard]] static auto get_match_warning(
        const AutoWlrRandrProfile & profile,
        const std::vector<ConnectedOutputInfo> & connected_outputs
    ) -> std::string;

    [[nodiscard]] static auto get_connected_outputs() -> std::vector<ConnectedOutputInfo>;
};
