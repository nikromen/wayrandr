#include <qobject.h>
#include <qtestcase.h>
#include <qtmetamacros.h>

#include <QList>
#include <QPoint>
#include <QTest>
#include <cstddef>
#include <limits>
#include <optional>
#include <vector>

#include "models/monitor_properties.hpp"
#include "models/profile/profile_output_properties.hpp"
#include "models/shared/canvas_participants.hpp"
#include "monitor_specs.hpp"
#include "utils/canvas_layout.hpp"

class DragCanvas : public QObject {
    Q_OBJECT
public:
    // Return the proposed position to observe drag arithmetic before canvas clamping.
    Q_INVOKABLE QPoint snap_position(
        QObject * /*unused*/,
        int x,
        int y,
        int /*unused*/,
        int /*unused*/,
        int /*unused*/,
        float /*unused*/
    ) {
        return { x, y };
    }
};

class LayoutTests : public QObject {
    Q_OBJECT
private slots:

    void dimensions_data() {
        QTest::addColumn<int>("pixels_x");
        QTest::addColumn<int>("pixels_y");
        QTest::addColumn<float>("scale");
        QTest::addColumn<bool>("rotated");
        QTest::addColumn<QPoint>("expected");
        const auto maximum = std::numeric_limits<int>::max();
        const auto infinity = std::numeric_limits<float>::infinity();
        QTest::newRow("normal") << 1920 << 1080 << 1.0F << false << QPoint(1920, 1080);
        QTest::newRow("scaled-rotation") << 3840 << 2160 << 2.0F << true << QPoint(1080, 1920);
        QTest::newRow("fractional") << 1920 << 1080 << 1.25F << false << QPoint(1536, 864);
        QTest::newRow("round-half") << 3 << 5 << 2.0F << false << QPoint(2, 3);
        QTest::newRow("maximum") << maximum << maximum << 1.0F << false << QPoint(maximum, maximum);
        QTest::newRow("nan") << 1920 << 1080 << std::numeric_limits<float>::quiet_NaN() << false
                             << QPoint(0, 0);
        QTest::newRow("positive-infinity") << 1920 << 1080 << infinity << false << QPoint(0, 0);
        QTest::newRow("negative-infinity") << 1920 << 1080 << -infinity << true << QPoint(0, 0);
        QTest::newRow("zero") << 1920 << 1080 << 0.0F << true << QPoint(0, 0);
        QTest::newRow("negative") << 1920 << 1080 << -1.0F << false << QPoint(0, 0);
        QTest::newRow("tiny") << 1920 << 1080 << std::numeric_limits<float>::denorm_min() << true
                              << QPoint(0, 0);
        QTest::newRow("unrepresentable") << maximum << 1080 << 0.5F << false << QPoint(0, 0);
        QTest::newRow("negative-pixels")
            << std::numeric_limits<int>::min() << 1080 << 1.0F << false << QPoint(0, 0);
        QTest::newRow("zero-pixels") << 0 << 0 << 1.0F << true << QPoint(0, 0);
        QTest::newRow("large-scale")
            << 1920 << 1080 << std::numeric_limits<float>::max() << false << QPoint(0, 0);
    }

    void dimensions() {
        QFETCH(int, pixels_x);
        QFETCH(int, pixels_y);
        QFETCH(float, scale);
        QFETCH(bool, rotated);
        QFETCH(QPoint, expected);
        const auto size = canvas_layout::compute_layout_size(pixels_x, pixels_y, scale, rotated);
        QCOMPARE(QPoint(size.width, size.height), expected);
    }

    void extreme_snap() {
        const auto minimum = std::numeric_limits<int>::min();
        const auto maximum = std::numeric_limits<int>::max();
        const canvas_layout::CanvasRect high{ maximum, maximum, 1920, 1080, true };
        const canvas_layout::CanvasRect low{ minimum, minimum, maximum, maximum, true };
        const canvas_layout::CanvasRect huge{ maximum, maximum, maximum, maximum, true };
        QCOMPARE(
            canvas_layout::snap_position(high, { low }, 200, 0, 0, 1), QPoint(maximum, maximum)
        );
        QCOMPARE(canvas_layout::snap_position(low, { high }, 200, 0, 0, 1), QPoint(0, 0));
        QCOMPARE(
            canvas_layout::snap_position(huge, { high, low }, 200, 0, 0, 1),
            QPoint(maximum, maximum)
        );
        const canvas_layout::CanvasRect origin{ 0, 0, 0, 0, true };
        QCOMPARE(
            canvas_layout::snap_position(
                origin, { { minimum, minimum, 0, 0, true } }, maximum, 0, 0, 1
            ),
            QPoint(0, 0)
        );
        QCOMPARE(
            canvas_layout::snap_position(
                high, { { maximum, maximum, 20, 20, true } }, 200, 0, 0, 1
            ),
            QPoint(maximum, maximum)
        );
        QCOMPARE(
            canvas_layout::snap_position(
                high, { { maximum - 100, maximum - 100, 1920, 1080, true } }, 200, 0, 0, 1
            ),
            QPoint(maximum - 100, maximum - 100)
        );
        QCOMPARE(
            canvas_layout::snap_position(high, {}, 200, maximum, maximum, 1),
            QPoint(maximum - 1920, maximum - 1080)
        );
        for (const float scale : { std::numeric_limits<float>::quiet_NaN(),
                                   std::numeric_limits<float>::infinity(),
                                   -std::numeric_limits<float>::infinity(),
                                   0.0F,
                                   -1.0F,
                                   std::numeric_limits<float>::denorm_min() }) {
            QCOMPARE(
                canvas_layout::snap_position(high, {}, 200, maximum, maximum, scale),
                QPoint(maximum, maximum)
            );
        }
        QCOMPARE(
            canvas_layout::snap_position(
                high, {}, 200, maximum, maximum, std::numeric_limits<float>::max()
            ),
            QPoint(0, 0)
        );
        const canvas_layout::CanvasRect inactive{ minimum, maximum, maximum, maximum, false };
        QCOMPARE(
            canvas_layout::snap_position(inactive, { high }, 200, 1, 1, 1), QPoint(minimum, maximum)
        );
    }

    void extreme_drag() {
        const auto minimum = std::numeric_limits<int>::min();
        const auto maximum = std::numeric_limits<int>::max();
        DragCanvas canvas;
        ProfileOutputProperties output(nullptr);
        output.set_position_x(-50);
        output.set_position_y(-50);
        output.start_drag(minimum, maximum);
        output.update_drag(maximum, minimum, 1, &canvas, 0, 0);
        QCOMPARE(
            QPoint(output.get_position_x(), output.get_position_y()), QPoint(maximum, minimum)
        );
        output.start_drag(0, 0);
        output.update_drag(1, -1, 1, &canvas, 0, 0);
        QCOMPARE(
            QPoint(output.get_position_x(), output.get_position_y()), QPoint(maximum, minimum)
        );
        // Position addition and tiny-scale deltas saturate too, not just mouse subtraction.
        output.start_drag(0, 0);
        output.update_drag(1, -1, std::numeric_limits<float>::denorm_min(), &canvas, 0, 0);
        QCOMPARE(
            QPoint(output.get_position_x(), output.get_position_y()), QPoint(maximum, minimum)
        );
        for (const float scale : { std::numeric_limits<float>::quiet_NaN(),
                                   std::numeric_limits<float>::infinity(),
                                   -std::numeric_limits<float>::infinity(),
                                   0.0F,
                                   -1.0F }) {
            output.update_drag(minimum, maximum, scale, &canvas, 0, 0);
            QCOMPARE(
                QPoint(output.get_position_x(), output.get_position_y()), QPoint(maximum, minimum)
            );
        }
        output.set_position_x(-100);
        output.set_position_y(20);
        output.start_drag(0, 0);
        output.update_drag(3, -3, 2, &canvas, 0, 0);
        QCOMPARE(QPoint(output.get_position_x(), output.get_position_y()), QPoint(-99, 19));
    }

    void extreme_reset() {
        const auto maximum = std::numeric_limits<int>::max();
        std::vector<canvas_layout::ResettableItem> items = {
            { maximum - 100, -100, 50, true }, { maximum, -7, -8, false }, { 200, 0, 0, true },
            { maximum, 0, 0, true },           { -1, 0, 0, true },         { maximum, 0, 0, true }
        };
        canvas_layout::reset_horizontal_layout(items);
        QCOMPARE(QPoint(items[0].position_x, items[0].position_y), QPoint(0, 0));
        QCOMPARE(QPoint(items[1].position_x, items[1].position_y), QPoint(-7, -8));
        QCOMPARE(QPoint(items[2].position_x, items[2].position_y), QPoint(maximum - 100, 0));
        for (size_t index = 3; index < items.size(); ++index) {
            QCOMPARE(QPoint(items[index].position_x, items[index].position_y), QPoint(maximum, 0));
        }
    }

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
        MonitorProperties first(first_specs);
        MonitorProperties rotated(rotated_specs);
        MonitorProperties disabled(disabled_specs);
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
