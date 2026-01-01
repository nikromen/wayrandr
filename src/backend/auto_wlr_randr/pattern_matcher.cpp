#include "backend/auto_wlr_randr/pattern_matcher.hpp"

#include <fnmatch.h>

#include <nlohmann/json.hpp>
#include <vector>

#include "monitor_specs.hpp"
#include "utils/helpers.hpp"

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

auto monitor_specs_to_connected_output(const MonitorSpecs & monitor) -> ConnectedOutputInfo {
    ConnectedOutputInfo output;
    output.name = monitor.get_name();
    output.make = optional_non_empty(monitor.get_make());
    output.model = optional_non_empty(monitor.get_model());
    output.serial = optional_non_empty(monitor.get_serial_number());
    return output;
}

}  // namespace

auto AutoWlrRandrPatternMatcher::matches_pattern(
    const std::string & pattern, const ConnectedOutputInfo & output
) -> bool {
    if (!is_valid_glob_pattern(pattern)) {
        return false;
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

auto AutoWlrRandrPatternMatcher::find_matching_output(
    const std::string & pattern, const std::vector<ConnectedOutputInfo> & outputs
) -> std::optional<ConnectedOutputInfo> {
    for (const auto & output : outputs) {
        if (matches_pattern(pattern, output)) {
            return output;
        }
    }
    return std::nullopt;
}

auto AutoWlrRandrPatternMatcher::would_profile_match(
    const AutoWlrRandrProfile & profile, const std::vector<ConnectedOutputInfo> & connected_outputs
) -> bool {
    if (profile.settings.size() != connected_outputs.size()) {
        return false;
    }

    if (profile.settings.empty()) {
        return connected_outputs.empty();
    }

    std::vector<bool> used_outputs(connected_outputs.size(), false);

    for (const auto & setting : profile.settings) {
        if (!is_valid_glob_pattern(setting.output)) {
            return false;
        }

        bool found = false;
        for (size_t i = 0; i < connected_outputs.size(); ++i) {
            if (used_outputs[i]) {
                continue;
            }

            if (matches_pattern(setting.output, connected_outputs[i])) {
                used_outputs[i] = true;
                found = true;
                break;
            }
        }

        if (!found) {
            return false;
        }
    }

    return true;
}

auto AutoWlrRandrPatternMatcher::get_match_warning(
    const AutoWlrRandrProfile & profile, const std::vector<ConnectedOutputInfo> & connected_outputs
) -> std::string {
    if (profile.settings.size() != connected_outputs.size()) {
        return "Profile has " + std::to_string(profile.settings.size()) + " outputs but " +
            std::to_string(connected_outputs.size()) + " are connected. Auto-match will not apply.";
    }

    if (would_profile_match(profile, connected_outputs)) {
        return {};
    }

    return "Profile patterns do not match the currently connected outputs.";
}

auto AutoWlrRandrPatternMatcher::get_connected_outputs() -> std::vector<ConnectedOutputInfo> {
    const auto monitors = get_monitor_specs_list();
    std::vector<ConnectedOutputInfo> outputs;
    outputs.reserve(monitors.size());

    for (const auto & monitor : monitors) {
        outputs.push_back(monitor_specs_to_connected_output(monitor));
    }

    return outputs;
}
