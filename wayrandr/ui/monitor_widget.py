import copy
import shutil
import tempfile
from subprocess import run

from PySide6.QtCore import QPoint, Qt
from PySide6.QtGui import QImage, QPainter, QPixmap, QTransform
from PySide6.QtWidgets import (
    QFrame,
    QLabel,
)

from wayrandr.monitors import Monitor


class MonitorWidget(QFrame):
    def __init__(self, monitor: Monitor) -> None:
        super().__init__()
        self.monitor = monitor
        self.initial_monitor = copy.deepcopy(monitor)

        self.setup_frame()

        self.name_label = QLabel(self.monitor.name, self)
        self.name_label.move(10, 10)

        # TODO: getting img from xdg-desktop-portal every 1sec would be cool
        self.screenshot = None
        self.update_screen()

        self.setMouseTracking(True)
        self.drag_start_position = None

    def setup_frame(self) -> None:
        self.setFrameShape(QFrame.Box)
        self.setFixedSize(*self.monitor.active_mode.scaled_resolution())
        self.setCursor(Qt.CursorShape.OpenHandCursor)

    # ruff: noqa: N802 - paintEvent is a PyQt6 method
    def paintEvent(self, event) -> None:
        if self.screenshot is None:
            return

        pixmap = QPixmap(self.screenshot)
        painter = QPainter(self)
        painter.drawPixmap(0, 0, self.width(), self.height(), pixmap)

    # ruff: noqa: N802 - mousePressEvent is a PyQt6 method
    def mousePressEvent(self, event: QPoint) -> None:
        self.drag_start_position = event.pos()

        if event.button() == Qt.LeftButton:
            self.window().change_monitor_info_tab(self.monitor.name)

    # ruff: noqa: N802 - mouseMoveEvent is a PyQt6 method
    def mouseMoveEvent(self, event: QPoint) -> None:
        if event.buttons() == Qt.LeftButton:
            drag_distance = event.pos() - self.drag_start_position
            new_position = self.pos() + drag_distance
            self.move(new_position)
            # jump to MainWindow to get info about nearby monitors and do snap there
            # TODO: can qt do this in a better way?
            self.window().update_monitor_info_positions(self)
            self.window().snap_to_nearby_monitors(self)

    def update_screen(self):
        if not shutil.which("grim"):
            return

        temp_file = tempfile.NamedTemporaryFile(delete=True, suffix=".png")
        position = f"{self.monitor.position.x},{self.monitor.position.y}"
        resolution = f"{self.monitor.width}x{self.monitor.height}"
        run(
            [
                "grim",
                "-o",
                self.initial_monitor.name,
                "-g",
                f"{position} {resolution}",
                temp_file.name,
            ],
        )
        self.screenshot = QImage(temp_file.name)

    def rotate_screenshot(self, angle: int) -> None:
        if self.screenshot is None:
            return

        self.screenshot = self.screenshot.transformed(QTransform().rotate(angle))
        self.update()

    def mirror_screenshot(self) -> None:
        if self.screenshot is None:
            return

        self.screenshot = self.screenshot.transformed(QTransform().scale(-1, 1))
        self.update()
