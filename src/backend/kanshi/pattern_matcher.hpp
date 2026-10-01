#pragma once

#include <optional>
#include <string>
#include <vector>

#include "backend/kanshi/types.hpp"

class KanshiPatternMatcher {
public:
    [[nodiscard]] static auto matches_pattern(
        const std::string & pattern, const KanshiConnectedOutputInfo & output
    ) -> bool;
    [[nodiscard]] static auto find_matching_output(
        const std::string & pattern, const std::vector<KanshiConnectedOutputInfo> & outputs
    ) -> std::optional<KanshiConnectedOutputInfo>;
    [[nodiscard]] static auto would_profile_match(
        const KanshiProfile & profile,
        const std::vector<KanshiConnectedOutputInfo> & connected_outputs
    ) -> bool;
    [[nodiscard]] static auto get_match_warning(
        const KanshiProfile & profile,
        const std::vector<KanshiConnectedOutputInfo> & connected_outputs
    ) -> std::string;
    [[nodiscard]] static auto get_connected_outputs() -> std::vector<KanshiConnectedOutputInfo>;
};
