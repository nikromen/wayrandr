#include "backend/profile/matching.hpp"

#include <fnmatch.h>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "backend/profile/types.hpp"

namespace profile::matching {
namespace {

auto is_valid_glob_pattern(const std::string & pattern) -> bool {
    int bracket_depth = 0;
    for (const char character : pattern) {
        if (character == '[') {
            ++bracket_depth;
        } else if (character == ']') {
            --bracket_depth;
            if (bracket_depth < 0) {
                return false;
            }
        }
    }
    return bracket_depth == 0;
}

// Reassign an earlier selector when it occupies the only output a later one can use.
auto assign_output(
    const std::vector<std::string> & selectors,
    const std::vector<ConnectedOutput> & outputs,
    size_t selector_index,
    std::vector<std::optional<size_t>> & assigned,
    std::vector<bool> & visited,
    InvalidGlobBehavior invalid_glob
) -> bool {
    for (size_t index = 0; index < outputs.size(); ++index) {
        if (visited[index] ||
            !matches_pattern(selectors[selector_index], outputs[index], invalid_glob)) {
            continue;
        }
        visited[index] = true;
        if (!assigned[index] ||
            assign_output(selectors, outputs, *assigned[index], assigned, visited, invalid_glob)) {
            assigned[index] = selector_index;
            return true;
        }
    }
    return false;
}

}  // namespace

auto matches_pattern(
    const std::string & pattern, const ConnectedOutput & output, InvalidGlobBehavior invalid_glob
) -> bool {
    if (!is_valid_glob_pattern(pattern)) {
        return invalid_glob == InvalidGlobBehavior::LITERAL_NAME && pattern == output.name;
    }
    if (fnmatch(pattern.c_str(), output.name.c_str(), 0) == 0) {
        return true;
    }
    if (const auto identifier = output.build_identifier()) {
        if (fnmatch(pattern.c_str(), identifier->c_str(), 0) == 0) {
            return true;
        }
    }
    if (output.serial.has_value() && !output.serial->empty()) {
        if (fnmatch(pattern.c_str(), output.serial->c_str(), 0) == 0) {
            return true;
        }
    }
    return false;
}

auto find_matching_output(
    const std::string & pattern,
    const std::vector<ConnectedOutput> & outputs,
    InvalidGlobBehavior invalid_glob
) -> std::optional<ConnectedOutput> {
    for (const auto & output : outputs) {
        if (matches_pattern(pattern, output, invalid_glob)) {
            return output;
        }
    }
    return std::nullopt;
}

auto would_profile_match(
    const ProfileDefinition & profile,
    const std::vector<ConnectedOutput> & connected_outputs,
    InvalidGlobBehavior invalid_glob
) -> bool {
    if (profile.outputs.size() != connected_outputs.size()) {
        return false;
    }
    if (profile.outputs.empty()) {
        return connected_outputs.empty();
    }
    std::vector<std::string> selectors;
    selectors.reserve(profile.outputs.size());
    for (const auto & output : profile.outputs) {
        selectors.push_back(output.output);
    }
    std::vector<std::optional<size_t>> assigned(connected_outputs.size());
    for (size_t index = 0; index < selectors.size(); ++index) {
        std::vector<bool> visited(connected_outputs.size(), false);
        if (!assign_output(selectors, connected_outputs, index, assigned, visited, invalid_glob)) {
            return false;
        }
    }
    return true;
}

auto get_match_warning(
    const ProfileDefinition & profile,
    const std::vector<ConnectedOutput> & connected_outputs,
    InvalidGlobBehavior invalid_glob
) -> std::string {
    if (profile.outputs.size() != connected_outputs.size()) {
        return "Profile has " + std::to_string(profile.outputs.size()) + " outputs but " +
            std::to_string(connected_outputs.size()) + " are connected. Auto-match will not apply.";
    }
    if (would_profile_match(profile, connected_outputs, invalid_glob)) {
        return {};
    }
    return "Profile patterns do not match the currently connected outputs.";
}

}  // namespace profile::matching
