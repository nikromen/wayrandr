#include "backend.hpp"

#include <spdlog/spdlog.h>

#include <exception>
#include <functional>
#include <stdexcept>
#include <vector>

#include "backend/base.hpp"
#include "monitor_specs.hpp"

BackendManager::BackendManager()
    : current_type_(BackendType::WLR_RANDR),
      pending_confirmation_(false) {
    backend_ = create_backend(BackendType::WLR_RANDR);
}

void BackendManager::set_backend(BackendType type) {
    spdlog::info("Switching backend to type: {}", static_cast<int>(type));
    current_type_ = type;
    backend_ = create_backend(type);
}

void BackendManager::apply(const std::vector<MonitorSpecs> & monitors) {
    if (!backend_) {
        throw std::runtime_error("No backend available");
    }

    backend_->apply(monitors);
}

void BackendManager::revert() {
    if (!backend_) {
        throw std::runtime_error("No backend available");
    }

    backend_->revert();
}

void BackendManager::apply_with_confirmation(
    const std::vector<MonitorSpecs> & monitors,
    const std::function<void()> & /*on_timeout*/,
    int timeout_seconds
) {
    spdlog::info("Applying configuration with {} second confirmation timeout", timeout_seconds);

    try {
        apply(monitors);
        pending_confirmation_ = true;
    } catch (const std::exception & e) {
        spdlog::error("Failed to apply configuration: {}", e.what());
        pending_confirmation_ = false;
        throw;
    }
}

void BackendManager::confirm_apply() {
    if (!pending_confirmation_) {
        spdlog::warn("No pending configuration to confirm");
        return;
    }

    spdlog::info("Configuration confirmed by user");
    pending_confirmation_ = false;
}

void BackendManager::cancel_apply() {
    if (!pending_confirmation_) {
        spdlog::warn("No pending configuration to cancel");
        return;
    }

    spdlog::info("Configuration cancelled by user, reverting");
    pending_confirmation_ = false;
    revert();
}
