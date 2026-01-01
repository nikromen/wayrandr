#include "models/shared/canvas_participants.hpp"

#include "models/monitor_properties.hpp"
#include "models/profile/profile_editor_controller.hpp"
#include "models/profile/profile_output_properties.hpp"

namespace canvas_participants {

auto build_monitor_snap_context(MonitorProperties * current, const QList<QObject *> & monitors)
    -> SnapContext {
    SnapContext context;
    context.current_index = 0;

    for (QObject * monitor_obj : monitors) {
        auto * monitor = qobject_cast<MonitorProperties *>(monitor_obj);
        if (monitor == nullptr) {
            continue;
        }

        const bool is_current = monitor == current;
        if (is_current) {
            context.current_index = context.rects.size();
        }

        const bool active = monitor->is_enabled() && monitor->has_settings();
        context.rects.push_back(
            {
                is_current ? 0 : monitor->get_position_x(),
                is_current ? 0 : monitor->get_position_y(),
                monitor->get_layout_width(),
                monitor->get_layout_height(),
                active,
            }
        );
    }

    return context;
}

auto build_profile_snap_context(
    ProfileOutputProperties * current, ProfileEditorController * profile_editor
) -> SnapContext {
    SnapContext context;
    context.current_index = 0;

    for (QObject * output_obj : profile_editor->get_outputs()) {
        auto * output = qobject_cast<ProfileOutputProperties *>(output_obj);
        if (output == nullptr) {
            continue;
        }

        const bool is_current = output == current;
        if (is_current) {
            context.current_index = context.rects.size();
        }

        context.rects.push_back(
            {
                is_current ? 0 : output->get_position_x(),
                is_current ? 0 : output->get_position_y(),
                output->get_layout_width(),
                output->get_layout_height(),
                output->is_enabled(),
            }
        );
    }

    return context;
}

auto build_monitor_reset_context(const QList<QObject *> & monitors) -> ResetContext {
    ResetContext context;

    for (QObject * monitor_obj : monitors) {
        auto * monitor = qobject_cast<MonitorProperties *>(monitor_obj);
        if (monitor == nullptr) {
            continue;
        }

        context.items.push_back(
            {
                monitor->get_layout_width(),
                monitor->get_position_x(),
                monitor->get_position_y(),
                monitor->is_enabled() && monitor->has_settings(),
            }
        );
    }

    return context;
}

auto build_profile_reset_context(ProfileEditorController * profile_editor) -> ResetContext {
    ResetContext context;

    for (QObject * output_obj : profile_editor->get_outputs()) {
        auto * output = qobject_cast<ProfileOutputProperties *>(output_obj);
        if (output == nullptr) {
            continue;
        }

        context.items.push_back(
            {
                output->get_layout_width(),
                output->get_position_x(),
                output->get_position_y(),
                output->is_enabled(),
            }
        );
    }

    return context;
}

void apply_monitor_reset(const QList<QObject *> & monitors, const ResetContext & context) {
    for (int index = 0; index < monitors.size(); ++index) {
        auto * monitor = qobject_cast<MonitorProperties *>(monitors[index]);
        if (monitor == nullptr || index >= static_cast<int>(context.items.size())) {
            continue;
        }

        monitor->set_position_x(context.items[index].position_x);
        monitor->set_position_y(context.items[index].position_y);
    }
}

void apply_profile_reset(ProfileEditorController * profile_editor, const ResetContext & context) {
    const QList<QObject *> outputs = profile_editor->get_outputs();
    for (int index = 0; index < outputs.size(); ++index) {
        auto * output = qobject_cast<ProfileOutputProperties *>(outputs[index]);
        if (output == nullptr || index >= static_cast<int>(context.items.size())) {
            continue;
        }

        output->set_position_x(context.items[index].position_x);
        output->set_position_y(context.items[index].position_y);
    }
}

}  // namespace canvas_participants
