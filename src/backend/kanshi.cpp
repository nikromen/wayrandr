#include "backend/kanshi.hpp"

#include <spdlog/spdlog.h>

#include <string>
#include <vector>

#include "backend/base.hpp"
#include "monitor_specs.hpp"

KanshiBackend::KanshiBackend() {
    spdlog::debug("Initializing Kanshi backend");
}

void KanshiBackend::apply(const std::vector<MonitorSpecs> & /*monitors*/) {
    spdlog::warn("KanshiBackend::apply - Please implement me!");
}

void KanshiBackend::revert() {
    spdlog::warn("KanshiBackend::revert - Please implement me!");
}

void KanshiBackend::save(const Profile & /*profile*/) {
    spdlog::warn("KanshiBackend::save - Please implement me!");
}

void KanshiBackend::delete_profile(const std::string & /*name*/) {
    spdlog::warn("KanshiBackend::delete_profile - Please implement me!");
}

auto KanshiBackend::get_config_path() const -> std::string {
    spdlog::warn("KanshiBackend::get_config_path - Please implement me!");
    return "";
}
