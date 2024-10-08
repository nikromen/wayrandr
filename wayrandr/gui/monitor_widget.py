import copy
import shutil
import tempfile
from subprocess import run

from PySide6.QtCore import QPoint, Qt
from PySide6.QtGui import QImage, QPainter, QPixmap, QTransform
from PySide6.QtWidgets import QFrame

from wayrandr.gui.ui.generated_ui.monitor_widget import Ui_monitor_widget
from wayrandr.monitor import Monitor


class MonitorWidget(QFrame):
    def __init__(self, monitor: Monitor) -> None:
        super().__init__()
        self.monitor = monitor
        self.initial_monitor = copy.deepcopy(monitor)

        self.ui = Ui_monitor_widget()
        self.ui.setupUi(self)

        self.setFixedSize(*self.monitor.active_mode.scaled_resolution())

        self.ui.monitor_name_label.setText(self.monitor.name)

        # TODO: getting img from xdg-desktop-portal every 1sec would be cool
        # and it will get rid of that 1s delay at the start due to screenshoting
        self.screenshot = None
        self.update_screen()

        self.setMouseTracking(True)
        self.drag_start_position = None

    # ruff: noqa: N802 - paintEvent is a PyQt6 method
    def paintEvent(self, _) -> None:
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
