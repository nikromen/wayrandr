#include "backend/auto_wlr_randr.hpp"

#include <spdlog/spdlog.h>

#include <string>
#include <vector>

#include "backend/base.hpp"
#include "monitor_specs.hpp"

AutoWlrRandrBackend::AutoWlrRandrBackend() {
    spdlog::warn("AutoWlrRandrBackend - Please implement me!");
}

auto AutoWlrRandrBackend::get_config_path() const -> std::string {
    spdlog::warn("AutoWlrRandrBackend::get_config_path - Please implement me!");
    return "";
}

auto AutoWlrRandrBackend::load_profiles() -> void {
    spdlog::warn("AutoWlrRandrBackend::load_profiles - Please implement me!");
}

void AutoWlrRandrBackend::apply(const std::vector<MonitorSpecs> & /*monitors*/) {
    spdlog::warn("AutoWlrRandrBackend::apply - Please implement me!");
}

void AutoWlrRandrBackend::revert() {
    spdlog::warn("AutoWlrRandrBackend::revert - Please implement me!");
}

void AutoWlrRandrBackend::save(const Profile & /*profile*/) {
    spdlog::warn("AutoWlrRandrBackend::save - Please implement me!");
}

void AutoWlrRandrBackend::delete_profile(const std::string & /*name*/) {
    spdlog::warn("AutoWlrRandrBackend::delete_profile - Please implement me!");
}
