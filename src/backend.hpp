#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "backend/auto_wlr_randr/profile_backend.hpp"
#include "backend/base.hpp"
#include "backend/kanshi/profile_backend.hpp"
#include "backend/profile/editor_backend.hpp"
#include "monitor_specs.hpp"

class BackendManager {
public:
    BackendManager();
    ~BackendManager() = default;

    void set_backend(BackendType type);
    [[nodiscard]] auto get_backend_type() const -> BackendType;

    void apply(const std::vector<MonitorSpecs> & monitors);
    void revert();

    void apply_with_confirmation(
        const std::vector<MonitorSpecs> & monitors,
        const std::function<void()> & on_timeout,
        int timeout_seconds = 15
    );
    void confirm_apply();
    void cancel_apply();
    [[nodiscard]] auto has_pending_changes() const -> bool;
    [[nodiscard]] auto can_confirm() const -> bool;

    [[nodiscard]] auto profile_editor() -> ProfileEditorBackend &;
    [[nodiscard]] auto profile_editor() const -> const ProfileEditorBackend &;

private:
    void set_active_profile_editor(BackendType type);

    std::unique_ptr<Backend> display_backend_;
    AutoWlrRandrProfileBackend auto_wlr_randr_profile_backend_;
    KanshiProfileBackend kanshi_profile_backend_;
    ProfileEditorBackend * active_profile_editor_ = nullptr;
    BackendType current_type_;
    bool pending_confirmation_;
};
