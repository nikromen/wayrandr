#include "backend.hpp"

#include <spdlog/spdlog.h>

#include <exception>
#include <functional>
#include <stdexcept>
#include <vector>

#include "backend/base.hpp"
#include "monitor_specs.hpp"

BackendManager::BackendManager()
    : active_profile_editor_(&auto_wlr_randr_profile_backend_),
      current_type_(BackendType::WLR_RANDR),
      pending_confirmation_(false) {
    display_backend_ = create_display_backend(BackendType::WLR_RANDR);
}

void BackendManager::set_backend(BackendType type) {
    if (has_pending_changes()) {
        throw std::logic_error("Cannot switch backend while a configuration is pending");
    }
    spdlog::info("Switching backend to type: {}", static_cast<int>(type));
    current_type_ = type;
    display_backend_ = create_display_backend(type);
    set_active_profile_editor(type);
}

void BackendManager::set_active_profile_editor(BackendType type) {
    switch (type) {
        case BackendType::KANSHI:
            active_profile_editor_ = &kanshi_profile_backend_;
            break;
        case BackendType::AUTO_WLR_RANDR:
        case BackendType::WLR_RANDR:
        default:
            active_profile_editor_ = &auto_wlr_randr_profile_backend_;
            break;
    }
}

auto BackendManager::get_backend_type() const -> BackendType {
    return current_type_;
}

auto BackendManager::profile_editor() -> ProfileEditorBackend & {
    return *active_profile_editor_;
}

auto BackendManager::profile_editor() const -> const ProfileEditorBackend & {
    return *active_profile_editor_;
}

void BackendManager::apply(const std::vector<MonitorSpecs> & monitors) {
    if (!display_backend_) {
        throw std::runtime_error("No backend available");
    }

    if (has_pending_changes()) {
        throw std::logic_error("A configuration is already pending");
    }
    display_backend_->apply(monitors);
}

void BackendManager::revert() {
    if (!display_backend_) {
        throw std::runtime_error("No backend available");
    }

    display_backend_->revert();
}

void BackendManager::apply_with_confirmation(
    const std::vector<MonitorSpecs> & monitors,
    const std::function<void()> & /*on_timeout*/,
    int timeout_seconds
) {
    spdlog::info("Applying configuration with {} second confirmation timeout", timeout_seconds);

    try {
        apply(monitors);
        // Direct wlr-randr transactions are owned by the display backend.
        pending_confirmation_ = current_type_ != BackendType::WLR_RANDR;
    } catch (const std::exception & e) {
        spdlog::error("Failed to apply configuration: {}", e.what());
        throw;
    }
}

void BackendManager::confirm_apply() {
    if (!can_confirm()) {
        spdlog::warn("No pending configuration to confirm");
        return;
    }

    spdlog::info("Configuration confirmed by user");
    display_backend_->confirm();
    pending_confirmation_ = false;
}

void BackendManager::cancel_apply() {
    if (!has_pending_changes()) {
        spdlog::warn("No pending configuration to cancel");
        return;
    }

    spdlog::info("Configuration cancelled by user, reverting");
    revert();
    pending_confirmation_ = false;
}

auto BackendManager::has_pending_changes() const -> bool {
    if (current_type_ == BackendType::WLR_RANDR) {
        return display_backend_->has_pending_changes();
    }
    return pending_confirmation_;
}

auto BackendManager::can_confirm() const -> bool {
    if (current_type_ == BackendType::WLR_RANDR) {
        return display_backend_->can_confirm();
    }
    return pending_confirmation_;
}
