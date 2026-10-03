#include "backend/kanshi/pattern_matcher.hpp"

#include <fnmatch.h>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "backend/kanshi/types.hpp"
#include "monitor_specs.hpp"

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

auto optional_non_empty(const std::optional<std::string> & value) -> std::optional<std::string> {
    if (!value.has_value() || value->empty()) {
        return std::nullopt;
    }
    return value;
}

auto monitor_specs_to_connected_output(const MonitorSpecs & monitor) -> KanshiConnectedOutputInfo {
    KanshiConnectedOutputInfo output;
    output.name = monitor.get_name();
    output.make = optional_non_empty(monitor.get_make());
    output.model = optional_non_empty(monitor.get_model());
    output.serial = optional_non_empty(monitor.get_serial_number());
    return output;
}

// Reassign an earlier selector when it occupies the only output a later one can use.
auto assign_output(
    const std::vector<std::string> & selectors,
    const std::vector<KanshiConnectedOutputInfo> & outputs,
    size_t selector_index,
    std::vector<std::optional<size_t>> & assigned,
    std::vector<bool> & visited
) -> bool {
    for (size_t index = 0; index < outputs.size(); ++index) {
        if (visited[index] ||
            !KanshiPatternMatcher::matches_pattern(selectors[selector_index], outputs[index])) {
            continue;
        }
        visited[index] = true;
        if (!assigned[index] ||
            assign_output(selectors, outputs, *assigned[index], assigned, visited)) {
            assigned[index] = selector_index;
            return true;
        }
    }
    return false;
}

}  // namespace

auto KanshiPatternMatcher::matches_pattern(
    const std::string & pattern, const KanshiConnectedOutputInfo & output
) -> bool {
    if (!is_valid_glob_pattern(pattern)) {
        return pattern == output.name;
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

auto KanshiPatternMatcher::find_matching_output(
    const std::string & pattern, const std::vector<KanshiConnectedOutputInfo> & outputs
) -> std::optional<KanshiConnectedOutputInfo> {
    for (const auto & output : outputs) {
        if (matches_pattern(pattern, output)) {
            return output;
        }
    }
    return std::nullopt;
}

auto KanshiPatternMatcher::would_profile_match(
    const KanshiProfile & profile, const std::vector<KanshiConnectedOutputInfo> & connected_outputs
) -> bool {
    if (profile.outputs.size() != connected_outputs.size()) {
        return false;
    }

    if (profile.outputs.empty()) {
        return connected_outputs.empty();
    }

    std::vector<std::string> selectors;
    selectors.reserve(profile.outputs.size());
    for (const auto & setting : profile.outputs) {
        selectors.push_back(setting.criteria);
    }
    std::vector<std::optional<size_t>> assigned(connected_outputs.size());
    for (size_t index = 0; index < selectors.size(); ++index) {
        std::vector<bool> visited(connected_outputs.size(), false);
        if (!assign_output(selectors, connected_outputs, index, assigned, visited)) {
            return false;
        }
    }

    return true;
}

auto KanshiPatternMatcher::get_match_warning(
    const KanshiProfile & profile, const std::vector<KanshiConnectedOutputInfo> & connected_outputs
) -> std::string {
    if (profile.outputs.size() != connected_outputs.size()) {
        return "Profile has " + std::to_string(profile.outputs.size()) + " outputs but " +
            std::to_string(connected_outputs.size()) + " are connected. Auto-match will not apply.";
    }

    if (would_profile_match(profile, connected_outputs)) {
        return {};
    }

    return "Profile patterns do not match the currently connected outputs.";
}

auto KanshiPatternMatcher::get_connected_outputs() -> std::vector<KanshiConnectedOutputInfo> {
    const auto monitors = get_monitor_specs_list();
    std::vector<KanshiConnectedOutputInfo> outputs;
    outputs.reserve(monitors.size());

    for (const auto & monitor : monitors) {
        outputs.push_back(monitor_specs_to_connected_output(monitor));
    }

    return outputs;
}
