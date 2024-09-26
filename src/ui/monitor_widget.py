from PyQt5.QtWidgets import (
    QLabel,
    QFrame,
)
from PyQt5.QtCore import Qt, QPoint, QTimer
from PyQt5.QtGui import QPainter, QGuiApplication, QFont, QPixmap, QImage, QTransform

from src.constants import GRID_SCALING
from src.monitors import Monitor

import cv2
import numpy as np
import tempfile
from subprocess import run
import shutil
import copy


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
        self.setCursor(Qt.OpenHandCursor)
        self.setStyleSheet(
            """
            QFrame {
                background-color: #f5f5f5;
                border: 1px solid #ccc;
            }
            QLabel {
                color: #333;
                font-size: 14px;
            }
        """
        )

    def paintEvent(self, event: QGuiApplication) -> None:
        if self.screenshot is None:
            return

        pixmap = QPixmap(self.screenshot)
        painter = QPainter(self)
        painter.drawPixmap(0, 0, self.width(), self.height(), pixmap)

    def mousePressEvent(self, event: QPoint) -> None:
        self.drag_start_position = event.pos()

        if event.button() == Qt.LeftButton:
            self.window().change_monitor_info_tab(self.monitor.name)

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
            ]
        )
        screenshot = cv2.imread(temp_file.name)
        screenshot = cv2.cvtColor(screenshot, cv2.COLOR_BGR2RGB)
        self.screenshot = QImage(
            screenshot,
            screenshot.shape[1],
            screenshot.shape[0],
            QImage.Format_RGB888,
        )

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
