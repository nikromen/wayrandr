#include "backend.hpp"

#include <spdlog/spdlog.h>

#include <stdexcept>
#include <utility>
#include <vector>

#include "backend/base.hpp"
#include "backend/profile/editor_backend.hpp"
#include "monitor_specs.hpp"

BackendManager::BackendManager()
    : active_profile_editor_(&auto_wlr_randr_profile_backend_),
      current_type_(editor_mode::WLR_RANDR) {
    display_backend_ = create_display_backend(editor_mode::WLR_RANDR);
}

void BackendManager::set_backend(editor_mode::Mode type) {
    if (has_pending_changes()) {
        throw std::logic_error("Cannot switch backend while a configuration is pending");
    }
    spdlog::info("Switching backend to type: {}", static_cast<int>(type));
    auto display_backend = create_display_backend(type);
    current_type_ = type;
    display_backend_ = std::move(display_backend);
    set_active_profile_editor(type);
}

void BackendManager::set_active_profile_editor(editor_mode::Mode type) {
    switch (type) {
        case editor_mode::KANSHI:
            active_profile_editor_ = &kanshi_profile_backend_;
            break;
        case editor_mode::AUTO_WLR_RANDR:
        case editor_mode::WLR_RANDR:
            active_profile_editor_ = &auto_wlr_randr_profile_backend_;
            break;
    }
}

auto BackendManager::get_backend_type() const -> editor_mode::Mode {
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
        throw std::runtime_error("Direct display operations are unavailable in this editor mode");
    }

    if (has_pending_changes()) {
        throw std::logic_error("A configuration is already pending");
    }
    display_backend_->apply(monitors);
}

void BackendManager::revert() {
    if (!display_backend_) {
        throw std::runtime_error("Direct display operations are unavailable in this editor mode");
    }

    display_backend_->revert();
}

void BackendManager::confirm_apply() {
    if (!display_backend_) {
        throw std::runtime_error("Direct display operations are unavailable in this editor mode");
    }
    if (!can_confirm()) {
        spdlog::warn("No pending configuration to confirm");
        return;
    }

    spdlog::info("Configuration confirmed by user");
    display_backend_->confirm();
}

void BackendManager::cancel_apply() {
    if (!display_backend_) {
        throw std::runtime_error("Direct display operations are unavailable in this editor mode");
    }
    if (!has_pending_changes()) {
        spdlog::warn("No pending configuration to cancel");
        return;
    }

    spdlog::info("Configuration cancelled by user, reverting");
    revert();
}

auto BackendManager::has_pending_changes() const -> bool {
    return display_backend_ && display_backend_->has_pending_changes();
}

auto BackendManager::can_confirm() const -> bool {
    return display_backend_ && display_backend_->can_confirm();
}
