#include <spdlog/spdlog.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include "backend/kanshi/config_parser.hpp"
#include "backend/kanshi/types.hpp"
#include "spdlog/common.h"

namespace {
auto same_scale(const std::optional<float> & lhs, const std::optional<float> & rhs) -> bool {
    if (lhs.has_value() != rhs.has_value()) {
        return false;
    }
    if (!lhs) {
        return true;
    }
    if (std::isnan(*lhs) && std::isnan(*rhs)) {
        return true;
    }
    // max_digits10 serialization must recover finite floats exactly, including zero.
    return *lhs == *rhs;
}

auto same_outputs(
    const std::vector<KanshiOutputSetting> & lhs, const std::vector<KanshiOutputSetting> & rhs
) -> bool {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (size_t index = 0; index < lhs.size(); ++index) {
        const auto & a = lhs[index];
        const auto & b = rhs[index];
        if (std::tie(
                a.criteria,
                a.multi_output,
                a.enabled,
                a.mode,
                a.preferred,
                a.position,
                a.transform,
                a.adaptive_sync,
                a.alias
            ) !=
                std::tie(
                    b.criteria,
                    b.multi_output,
                    b.enabled,
                    b.mode,
                    b.preferred,
                    b.position,
                    b.transform,
                    b.adaptive_sync,
                    b.alias
                ) ||
            !same_scale(a.scale, b.scale)) {
            return false;
        }
    }
    return true;
}

auto same_document(const KanshiConfig & a, const KanshiConfig & b) -> bool {
    if (a.includes != b.includes || a.preserved_directives != b.preserved_directives ||
        !same_outputs(a.global_outputs, b.global_outputs) ||
        a.profiles.size() != b.profiles.size()) {
        return false;
    }
    for (size_t index = 0; index < a.profiles.size(); ++index) {
        const auto & lhs = a.profiles[index];
        const auto & rhs = b.profiles[index];
        if (lhs.id != rhs.id || lhs.exec != rhs.exec || !same_outputs(lhs.outputs, rhs.outputs)) {
            return false;
        }
    }
    return true;
}
}  // namespace

// NOLINTNEXTLINE: libFuzzer requires this C entry point.
extern "C" auto LLVMFuzzerTestOneInput(const uint8_t * data, size_t size) -> int {
    spdlog::set_level(spdlog::level::off);
    if (size > 65536) {
        return 0;
    }
    KanshiConfig document;
    try {
        document =
            kanshi_config_parser::parse(std::string(reinterpret_cast<const char *>(data), size));
    } catch (const kanshi_config_parser::ParseError &) {
        return 0;
    }
    const auto serialized = kanshi_config_parser::serialize(document);
    // A ParseError here is a serializer defect, not an expected rejection.
    const auto reloaded = kanshi_config_parser::parse(serialized);
    if (!same_document(document, reloaded)) {
        std::abort();
    }
    return 0;
}
