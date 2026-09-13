#include "models/profile/profile_output_properties.hpp"

#include <string>

#include "backend/profile/layout_utils.hpp"
#include "models/profile/profile_editor_controller.hpp"
#include "utils/canvas_drag.hpp"
#include "utils/canvas_layout.hpp"
#include "utils/transform_list.hpp"

ProfileOutputProperties::ProfileOutputProperties(ProfileEditorController * editor, QObject * parent)
    : QObject(parent),
      editor_(editor) {}

void ProfileOutputProperties::load_from_output(const profile::ProfileOutputDefinition & output) {
    output_ = output;
    enabled_ = output.enabled.value_or(true);
    adaptive_sync_ = output.adaptive_sync.value_or(false);
    refresh_match_preview();
    emit output_pattern_changed();
    emit enabled_changed();
    emit mode_changed();
    emit preferred_changed();
    emit position_x_changed();
    emit position_y_changed();
    emit scale_changed();
    emit transform_changed();
    emit adaptive_sync_changed();
    notify_layout_changed();
}

auto ProfileOutputProperties::to_output() const -> profile::ProfileOutputDefinition {
    profile::ProfileOutputDefinition output = output_;
    output.enabled = enabled_;
    output.adaptive_sync = adaptive_sync_;
    output.pos = std::to_string(get_position_x()) + "," + std::to_string(get_position_y());
    output.left_of.reset();
    output.right_of.reset();
    output.above.reset();
    output.below.reset();
    return output;
}

void ProfileOutputProperties::refresh_match_preview() {
    if (editor_ == nullptr) {
        return;
    }

    match_preview_ = editor_->get_output_match_preview(output_.output);
    emit match_preview_changed();
}

auto ProfileOutputProperties::get_output_pattern() const -> QString {
    return QString::fromStdString(output_.output);
}

auto ProfileOutputProperties::is_enabled() const -> bool {
    return enabled_;
}

void ProfileOutputProperties::set_enabled(bool enabled) {
    if (enabled_ == enabled) {
        return;
    }

    enabled_ = enabled;
    emit enabled_changed();
    notify_layout_changed();
    mark_dirty();
}

auto ProfileOutputProperties::get_mode() const -> QString {
    return output_.mode.has_value() ? QString::fromStdString(output_.mode.value()) : QString();
}

auto ProfileOutputProperties::is_preferred() const -> bool {
    return output_.preferred;
}

auto ProfileOutputProperties::has_explicit_position() const -> bool {
    return profile::layout_utils::parse_position(output_.pos).has_value();
}

auto ProfileOutputProperties::get_position_x() const -> int {
    if (const auto pos = profile::layout_utils::parse_position(output_.pos); pos.has_value()) {
        return pos->first;
    }

    if (editor_ != nullptr) {
        return editor_->get_live_position_for_output(get_output_pattern()).x();
    }

    return 0;
}

auto ProfileOutputProperties::get_position_y() const -> int {
    if (const auto pos = profile::layout_utils::parse_position(output_.pos); pos.has_value()) {
        return pos->second;
    }

    if (editor_ != nullptr) {
        return editor_->get_live_position_for_output(get_output_pattern()).y();
    }

    return 0;
}

void ProfileOutputProperties::refresh_live_position() {
    if (has_explicit_position()) {
        return;
    }

    emit position_x_changed();
    emit position_y_changed();
}

auto ProfileOutputProperties::get_scale() const -> float {
    return output_.scale.value_or(1.0F);
}

auto ProfileOutputProperties::get_transform() const -> QString {
    return QString::fromStdString(output_.transform.value_or("normal"));
}

auto ProfileOutputProperties::is_adaptive_sync() const -> bool {
    return adaptive_sync_;
}

void ProfileOutputProperties::set_adaptive_sync(bool adaptive_sync) {
    if (adaptive_sync_ == adaptive_sync) {
        return;
    }

    adaptive_sync_ = adaptive_sync;
    emit adaptive_sync_changed();
    mark_dirty();
}

auto ProfileOutputProperties::get_layout_width() const -> int {
    return profile::layout_utils::layout_width(output_);
}

auto ProfileOutputProperties::get_layout_height() const -> int {
    return profile::layout_utils::layout_height(output_);
}

auto ProfileOutputProperties::get_match_preview() const -> QString {
    return match_preview_;
}

auto ProfileOutputProperties::get_transform_list() -> QStringList {
    return transform_list::as_qstring_list();
}

void ProfileOutputProperties::set_output_pattern(const QString & pattern) {
    if (get_output_pattern() == pattern) {
        return;
    }
    output_.output = pattern.toStdString();
    refresh_match_preview();
    emit output_pattern_changed();
    mark_dirty();
}

void ProfileOutputProperties::set_mode(const QString & mode) {
    const std::string mode_value = mode.toStdString();
    if (output_.mode == mode_value || (!output_.mode.has_value() && mode.isEmpty())) {
        return;
    }
    if (mode.isEmpty()) {
        output_.mode.reset();
    } else {
        output_.mode = mode_value;
    }
    emit mode_changed();
    notify_layout_changed();
    mark_dirty();
}

void ProfileOutputProperties::set_preferred(bool preferred) {
    if (output_.preferred == preferred) {
        return;
    }
    output_.preferred = preferred;
    emit preferred_changed();
    mark_dirty();
}

void ProfileOutputProperties::set_position_x(int x) {
    const int y = get_position_y();
    const std::string new_pos = std::to_string(x) + "," + std::to_string(y);
    if (output_.pos == new_pos) {
        return;
    }
    output_.pos = new_pos;
    emit position_x_changed();
    mark_dirty();
}

void ProfileOutputProperties::set_position_y(int y) {
    const int x = get_position_x();
    const std::string new_pos = std::to_string(x) + "," + std::to_string(y);
    if (output_.pos == new_pos) {
        return;
    }
    output_.pos = new_pos;
    emit position_y_changed();
    mark_dirty();
}

void ProfileOutputProperties::set_scale(float scale) {
    if (output_.scale == scale) {
        return;
    }
    output_.scale = scale;
    emit scale_changed();
    notify_layout_changed();
    mark_dirty();
}

void ProfileOutputProperties::set_transform(const QString & transform) {
    const std::string next = transform.toStdString();
    if (output_.transform == next) {
        return;
    }
    output_.transform = next;
    emit transform_changed();
    notify_layout_changed();
    mark_dirty();
}

void ProfileOutputProperties::start_drag(int mouse_x, int mouse_y) {
    canvas_drag::start(drag_state_, mouse_x, mouse_y, get_position_x(), get_position_y());
}

void ProfileOutputProperties::update_drag(
    int mouse_x,
    int mouse_y,
    float display_scale,
    QObject * main_window,
    int canvas_width,
    int canvas_height
) {
    canvas_drag::update_with_snap(
        drag_state_,
        mouse_x,
        mouse_y,
        display_scale,
        this,
        main_window,
        canvas_width,
        canvas_height,
        canvas_layout::K_DEFAULT_SNAP_THRESHOLD
    );
}

void ProfileOutputProperties::notify_layout_changed() {
    emit layout_dimensions_changed();
}

void ProfileOutputProperties::mark_dirty() {
    emit setting_changed();
    if (editor_ != nullptr) {
        editor_->mark_dirty();
    }
}
