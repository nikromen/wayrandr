#pragma once

#include <qtmetamacros.h>

#include <QObject>
#include <functional>
#include <memory>
#include <vector>

#include "backend/auto_wlr_randr/profile_backend.hpp"
#include "backend/base.hpp"
#include "backend/kanshi/profile_backend.hpp"
#include "backend/profile/editor_backend.hpp"
#include "monitor_specs.hpp"

class BackendManager : public QObject, public std::enable_shared_from_this<BackendManager> {
    Q_OBJECT

public:
    BackendManager();

    // GUI-owned admission gate; worker operations never access this flag.
    [[nodiscard]] bool is_operation_busy() const { return operation_busy_; }

    void set_operation_busy(bool busy) {
        if (operation_busy_ == busy) {
            return;
        }

        operation_busy_ = busy;
        emit operation_busy_changed();
    }

    ~BackendManager() override = default;

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

signals:
    void operation_busy_changed();

private:
    bool operation_busy_ = false;
    void set_active_profile_editor(BackendType type);

    std::unique_ptr<Backend> display_backend_;
    AutoWlrRandrProfileBackend auto_wlr_randr_profile_backend_;
    KanshiProfileBackend kanshi_profile_backend_;
    ProfileEditorBackend * active_profile_editor_ = nullptr;
    BackendType current_type_;
    bool pending_confirmation_;
};
