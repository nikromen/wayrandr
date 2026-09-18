#pragma once

#include <qtmetamacros.h>

#include <QObject>
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

    void set_backend(editor_mode::Mode type);
    [[nodiscard]] auto get_backend_type() const -> editor_mode::Mode;

    // Direct operations throw in profile modes. MainWindow owns confirmation timers;
    // the display backend owns the snapshot until confirmation or successful recovery.
    void apply(const std::vector<MonitorSpecs> & monitors);
    void revert();

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
    void set_active_profile_editor(editor_mode::Mode type);

    std::unique_ptr<Backend> display_backend_;
    AutoWlrRandrProfileBackend auto_wlr_randr_profile_backend_;
    KanshiProfileBackend kanshi_profile_backend_;
    ProfileEditorBackend * active_profile_editor_ = nullptr;
    editor_mode::Mode current_type_;
};
