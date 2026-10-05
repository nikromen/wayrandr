#pragma once

#include <optional>
#include <string>
#include <vector>

#include "backend/profile/types.hpp"

namespace profile::matching {

enum class InvalidGlobBehavior { LITERAL_NAME, NO_MATCH };

[[nodiscard]] auto matches_pattern(
    const std::string & pattern, const ConnectedOutput & output, InvalidGlobBehavior invalid_glob
) -> bool;
[[nodiscard]] auto find_matching_output(
    const std::string & pattern,
    const std::vector<ConnectedOutput> & outputs,
    InvalidGlobBehavior invalid_glob
) -> std::optional<ConnectedOutput>;
[[nodiscard]] auto would_profile_match(
    const ProfileDefinition & profile,
    const std::vector<ConnectedOutput> & connected_outputs,
    InvalidGlobBehavior invalid_glob
) -> bool;
[[nodiscard]] auto get_match_warning(
    const ProfileDefinition & profile,
    const std::vector<ConnectedOutput> & connected_outputs,
    InvalidGlobBehavior invalid_glob
) -> std::string;

}  // namespace profile::matching
