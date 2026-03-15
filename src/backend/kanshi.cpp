#include "backend/kanshi.hpp"

#include <spdlog/spdlog.h>

#include <vector>

#include "monitor_specs.hpp"

KanshiBackend::KanshiBackend() {
    spdlog::debug("Initializing Kanshi backend");
}

void KanshiBackend::apply(const std::vector<MonitorSpecs> & /*monitors*/) {
    spdlog::warn("KanshiBackend::apply - Please implement me!");
    pending_confirmation_ = true;
}

void KanshiBackend::revert() {
    spdlog::warn("KanshiBackend::revert - Please implement me!");
    pending_confirmation_ = false;
}

void KanshiBackend::confirm() {
    pending_confirmation_ = false;
}

auto KanshiBackend::has_pending_changes() const -> bool {
    return pending_confirmation_;
}
