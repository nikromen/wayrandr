#include <qobject.h>
#include <qtestcase.h>
#include <qtmetamacros.h>

#include <QList>
#include <QPoint>
#include <QTest>
#include <optional>
#include <vector>

#include "models/monitor_properties.hpp"
#include "models/shared/canvas_participants.hpp"
#include "monitor_specs.hpp"
#include "utils/canvas_layout.hpp"

class LayoutTests : public QObject {
    Q_OBJECT
private slots:

    void desktop_layout() {
        MonitorSpecs first_specs(
            "DP-1",
            std::nullopt,
            std::nullopt,
            std::nullopt,
            "Desktop",
            { 300, 200 },
            true,
            { Mode(1920, 1080, 60, true, true) },
            EnabledMonitorSettings(0, { 80, 50 }, Transform::NORMAL, 1, false)
        );
        MonitorSpecs rotated_specs(
            "DP-2",
            std::nullopt,
            std::nullopt,
            std::nullopt,
            "Portrait",
            { 300, 200 },
            true,
            { Mode(3840, 2160, 60, true, true) },
            EnabledMonitorSettings(0, { 90, 40 }, Transform::ROTATE_90, 2, false)
        );
        MonitorSpecs disabled_specs(
            "HDMI-1",
            std::nullopt,
            std::nullopt,
            std::nullopt,
            "Off",
            { 300, 200 },
            false,
            { Mode(9999, 9999, 60, true, false) }
        );
        MonitorProperties first(&first_specs);
        MonitorProperties rotated(&rotated_specs);
        MonitorProperties disabled(&disabled_specs);
        const QList<QObject *> monitors = { &first, &disabled, &rotated };
        QCOMPARE(first.get_layout_width(), 1920);
        QCOMPARE(first.get_layout_height(), 1080);
        QCOMPARE(rotated.get_layout_width(), 1080);
        QCOMPARE(rotated.get_layout_height(), 1920);
        auto reset = canvas_participants::build_monitor_reset_context(monitors);
        canvas_layout::reset_horizontal_layout(reset.items);
        canvas_participants::apply_monitor_reset(monitors, reset);
        QCOMPARE(QPoint(first.get_position_x(), first.get_position_y()), QPoint(0, 0));
        QCOMPARE(QPoint(rotated.get_position_x(), rotated.get_position_y()), QPoint(1920, 0));
        const auto context = canvas_participants::build_monitor_snap_context(&rotated, monitors);
        const auto & moving = context.rects[context.current_index];
        QCOMPARE(
            canvas_layout::snap_from_rects(
                moving, context.rects, context.current_index, 1950, 90, 200, 4000, 2400, 1
            ),
            QPoint(1920, 0)
        );
        QCOMPARE(
            canvas_layout::snap_from_rects(
                moving, context.rects, context.current_index, 1950, 90, 200, 2000, 1200, 0.5F
            ),
            QPoint(1920, 0)
        );
        const std::vector<canvas_layout::CanvasRect> others = { { 0, 0, 1920, 1080, true },
                                                                { 1950, 90, 9999, 9999, false } };
        const canvas_layout::CanvasRect outside = { 9000, 9000, 1080, 1920, true };
        QCOMPARE(
            canvas_layout::snap_position(outside, others, 200, 2000, 1200, 0.5F), QPoint(2920, 480)
        );
        const canvas_layout::CanvasRect negative = { -500, -500, 1080, 1920, true };
        QCOMPARE(canvas_layout::snap_position(negative, {}, 200, 4000, 2400, 1), QPoint(0, 0));
    }
};
QTEST_GUILESS_MAIN(LayoutTests)
#include "layout.moc"
