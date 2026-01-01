#pragma once

#include <QList>
#include <QObject>
#include <cstddef>
#include <vector>

#include "utils/canvas_layout.hpp"

class MonitorProperties;
class ProfileEditorController;
class ProfileOutputProperties;

namespace canvas_participants {

struct SnapContext {
    std::vector<canvas_layout::CanvasRect> rects;
    size_t current_index = 0;
};

struct ResetContext {
    std::vector<canvas_layout::ResettableItem> items;
};

[[nodiscard]] auto build_monitor_snap_context(
    MonitorProperties * current, const QList<QObject *> & monitors
) -> SnapContext;

[[nodiscard]] auto build_profile_snap_context(
    ProfileOutputProperties * current, ProfileEditorController * profile_editor
) -> SnapContext;

[[nodiscard]] auto build_monitor_reset_context(const QList<QObject *> & monitors) -> ResetContext;

[[nodiscard]] auto build_profile_reset_context(ProfileEditorController * profile_editor)
    -> ResetContext;

void apply_monitor_reset(const QList<QObject *> & monitors, const ResetContext & context);

void apply_profile_reset(ProfileEditorController * profile_editor, const ResetContext & context);

}  // namespace canvas_participants
