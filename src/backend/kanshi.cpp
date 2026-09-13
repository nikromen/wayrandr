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
