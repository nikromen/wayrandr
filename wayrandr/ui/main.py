from PySide6.QtCore import QRect, Qt
from PySide6.QtWidgets import (
    QGridLayout,
    QGroupBox,
    QMainWindow,
    QPushButton,
    QTabWidget,
    QVBoxLayout,
    QWidget,
)

from wayrandr.constants import SNAP_DISTANCE
from wayrandr.helpers import deapply_scaling
from wayrandr.monitors import Monitor, Transform, get_monitors
from wayrandr.ui.monitor_info_widget import MonitorInfoWidget
from wayrandr.ui.monitor_widget import MonitorWidget


class MainWindow(QMainWindow):
    def __init__(self) -> None:
        super().__init__()

        self.setWindowTitle("Monitor Configuration")

        self.layout = QGridLayout()
        self.monitors_area = QWidget(self)
        self.monitors_area.setLayout(self.layout)

        self.monitor_widgets = self.set_monitor_widgets()

        self.save_button = QPushButton("Save", self)
        self.setup_save_button()

        self.main_layout = QVBoxLayout()
        self.main_layout.addWidget(self.monitors_area)
        self.main_layout.addWidget(self.save_button)

        self.central_widget = QWidget(self)
        self.central_widget.setLayout(self.main_layout)
        self.setCentralWidget(self.central_widget)

        self.monitor_details = QGroupBox("Monitor Details", self)
        self.monitor_details_layout = QVBoxLayout()
        self.setup_monitor_details()

        self.tab_widget = QTabWidget()
        self.monitor_info_widgets = self.set_tab_widget_for_monitor_details()

        self.monitor_details_layout.addWidget(self.tab_widget)

    def setup_monitor_details(self) -> None:
        self.monitor_details.setLayout(self.monitor_details_layout)

        self.monitor_details.move(800, 0)
        self.monitor_details.resize(400, 600)

    def setup_save_button(self) -> None:
        self.save_button.clicked.connect(self.save_configuration)
        self.save_button.setCursor(Qt.PointingHandCursor)

    def set_tab_widget_for_monitor_details(self) -> list[MonitorInfoWidget]:
        monitor_info_widgets = []
        for monitor in get_monitors():
            monitor_info_widget = MonitorInfoWidget(monitor)
            self.tab_widget.addTab(monitor_info_widget, monitor.name)
            monitor_info_widgets.append(monitor_info_widget)

        return monitor_info_widgets

    def set_monitor_widgets(self) -> list[MonitorWidget]:
        monitor_widgets = []
        for monitor in get_monitors():
            monitor_widget = MonitorWidget(monitor)
            monitor_widget.move(*monitor.position.scaled_position())
            monitor_widget.setParent(self.monitors_area)
            monitor_widget.show()
            monitor_widgets.append(monitor_widget)

        return monitor_widgets

    def save_configuration(self) -> None:
        pass

    def get_monitor_widget_by_name(
        self,
        name: str,
        is_monitor_info: bool = False,
    ) -> MonitorWidget | MonitorInfoWidget:
        widgets = self.monitor_widgets
        if is_monitor_info:
            widgets = self.monitor_info_widgets

        for widget in widgets:
            if widget.monitor.name == name:
                return widget

        raise ValueError(f"Monitor with name {name} not found")

    def update_monitor_positions(self, monitor: Monitor) -> None:
        monitor_widget = self.get_monitor_widget_by_name(monitor.name)
        monitor_widget.move(*monitor.position.scaled_position())

    def update_monitor_info_positions(self, monitor_widget: MonitorWidget) -> None:
        monitor_info = self.get_monitor_widget_by_name(
            monitor_widget.monitor.name,
            is_monitor_info=True,
        )
        # wtf, when set here it laggs?!
        monitor_info.set_position(
            int(deapply_scaling(monitor_widget.x())),
            int(deapply_scaling(monitor_widget.y())),
        )

    def update_monitor_resolution(self, monitor: Monitor) -> None:
        monitor_widget = self.get_monitor_widget_by_name(monitor.name)
        monitor_widget.setFixedSize(*monitor.active_mode.scaled_resolution())

    def change_monitor_info_tab(self, monitor_name: str) -> None:
        monitor_info = self.get_monitor_widget_by_name(monitor_name, is_monitor_info=True)
        self.tab_widget.setCurrentWidget(monitor_info)

    def update_monitor_scale(self, monitor: Monitor) -> None:
        monitor_widget = self.get_monitor_widget_by_name(monitor.name)
        monitor_widget.setFixedSize(*monitor.active_mode.scaled_resolution(monitor.scale))

    def _handle_rotation(
        self,
        monitor_widget: MonitorWidget,
        curr_transform: Transform,
        prev_transform: Transform,
    ) -> None:
        if monitor_widget.screenshot is None:
            return

        rotation_map = {
            Transform.normal: 0,
            Transform.normal_90: 90,
            Transform.normal_180: 180,
            Transform.normal_270: 270,
            Transform.flipped: 0,
            Transform.flipped_90: -90,
            Transform.flipped_180: 180,
            Transform.flipped_270: -270,
        }
        rotate = rotation_map[curr_transform] - rotation_map[prev_transform]
        monitor_widget.rotate_screenshot(rotate)

    def update_monitor_transform(self, monitor: Monitor, prev_transform: Transform) -> None:
        monitor_widget = self.get_monitor_widget_by_name(monitor.name)
        if prev_transform == monitor.transform:
            return

        if prev_transform.is_rotated != monitor.transform.is_rotated:
            monitor_widget.setFixedSize(monitor_widget.height(), monitor_widget.width())

        self._handle_rotation(monitor_widget, monitor.transform, prev_transform)

    def update_monitor_mirror(self, monitor_name: str) -> None:
        monitor_widget = self.get_monitor_widget_by_name(monitor_name)
        monitor_widget.mirror_screenshot()

    def snap_to_nearby_monitors(self, moving_monitor: MonitorWidget) -> None:
        for monitor in self.monitor_widgets:
            if monitor == moving_monitor:
                continue

            moving_monitor_rect = QRect(moving_monitor.pos(), moving_monitor.size())
            target_monitor_rect = QRect(monitor.pos(), monitor.size())

            # this did chatgpt, do not touch :D
            self.snap_if_near_horizontally(
                moving_monitor,
                moving_monitor_rect,
                target_monitor_rect,
                monitor,
            )
            self.snap_if_near_vertically(
                moving_monitor,
                moving_monitor_rect,
                target_monitor_rect,
                monitor,
            )
            self.snap_if_near_diagonally(
                moving_monitor,
                moving_monitor_rect,
                target_monitor_rect,
                monitor,
            )
            self.snap_edges(moving_monitor, moving_monitor_rect, target_monitor_rect, monitor)

    def snap_if_near_horizontally(
        self,
        moving_monitor,
        moving_monitor_rect,
        target_monitor_rect,
        monitor,
    ):
        if abs(moving_monitor_rect.right() - target_monitor_rect.left()) < SNAP_DISTANCE:
            moving_monitor.move(monitor.x() - moving_monitor.width(), moving_monitor.y())
        elif abs(moving_monitor_rect.left() - target_monitor_rect.right()) < SNAP_DISTANCE:
            moving_monitor.move(monitor.x() + monitor.width(), moving_monitor.y())

    def snap_if_near_vertically(
        self,
        moving_monitor,
        moving_monitor_rect,
        target_monitor_rect,
        monitor,
    ):
        if abs(moving_monitor_rect.bottom() - target_monitor_rect.top()) < SNAP_DISTANCE:
            moving_monitor.move(moving_monitor.x(), monitor.y() - moving_monitor.height())
        elif abs(moving_monitor_rect.top() - target_monitor_rect.bottom()) < SNAP_DISTANCE:
            moving_monitor.move(moving_monitor.x(), monitor.y() + monitor.height())

    def snap_if_near_diagonally(
        self,
        moving_monitor,
        moving_monitor_rect,
        target_monitor_rect,
        monitor,
    ):
        if (
            abs(moving_monitor_rect.top() - target_monitor_rect.bottom()) < SNAP_DISTANCE
            and abs(moving_monitor_rect.right() - target_monitor_rect.left()) < SNAP_DISTANCE
        ):
            moving_monitor.move(
                monitor.x() - moving_monitor.width(),
                monitor.y() + monitor.height(),
            )
        elif (
            abs(moving_monitor_rect.bottom() - target_monitor_rect.top()) < SNAP_DISTANCE
            and abs(moving_monitor_rect.left() - target_monitor_rect.right()) < SNAP_DISTANCE
        ):
            moving_monitor.move(
                monitor.x() + monitor.width(),
                monitor.y() - moving_monitor.height(),
            )
        elif (
            abs(moving_monitor_rect.top() - target_monitor_rect.bottom()) < SNAP_DISTANCE
            and abs(moving_monitor_rect.left() - target_monitor_rect.right()) < SNAP_DISTANCE
        ):
            moving_monitor.move(monitor.x() + monitor.width(), monitor.y() + monitor.height())
        elif (
            abs(moving_monitor_rect.bottom() - target_monitor_rect.top()) < SNAP_DISTANCE
            and abs(moving_monitor_rect.right() - target_monitor_rect.left()) < SNAP_DISTANCE
        ):
            moving_monitor.move(
                monitor.x() - moving_monitor.width(),
                monitor.y() - moving_monitor.height(),
            )

    def snap_edges(self, moving_monitor, moving_monitor_rect, target_monitor_rect, monitor):
        if abs(moving_monitor_rect.left() - target_monitor_rect.left()) < SNAP_DISTANCE:
            moving_monitor.move(monitor.x(), moving_monitor.y())
        elif abs(moving_monitor_rect.right() - target_monitor_rect.right()) < SNAP_DISTANCE:
            moving_monitor.move(
                monitor.x() + monitor.width() - moving_monitor.width(),
                moving_monitor.y(),
            )

        if abs(moving_monitor_rect.top() - target_monitor_rect.top()) < SNAP_DISTANCE:
            moving_monitor.move(moving_monitor.x(), monitor.y())
        elif abs(moving_monitor_rect.bottom() - target_monitor_rect.bottom()) < SNAP_DISTANCE:
            moving_monitor.move(
                moving_monitor.x(),
                monitor.y() + monitor.height() - moving_monitor.height(),
            )
