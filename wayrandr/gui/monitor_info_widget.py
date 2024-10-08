from PySide6.QtWidgets import (
    QWidget,
)

from wayrandr.gui.ui.generated_ui.monitor_info_widget import Ui_monitor_info_widget
from wayrandr.monitor import Monitor, Transform


class MonitorInfoWidget(QWidget):
    def __init__(self, monitor: Monitor) -> None:
        super().__init__()

        self.ui = Ui_monitor_info_widget()
        self.ui.setupUi(self)

        self.monitor = monitor

        self.setup_enabled_checkbox()
        self.setup_flipped_checkbox()
        self.setup_info_labels()
        self.setup_resolution_combobox()
        self.setup_scale_double_spinbox()
        self.setup_position_spinboxes()
        self.setup_transform_combobox()

    def setup_transform_combobox(self) -> None:
        # TODO: why items in UI results in empty string?
        self.ui.transform_combobox.addItems(
            [
                item
                for item in [
                    "Don't rotate",
                    "Rotate 90°",
                    "Rotate 180°",
                    "Rotate 270°",
                ]
            ],
        )
        self.ui.transform_combobox.setCurrentIndex(self.monitor.transform.value_index())
        self.ui.transform_combobox.activated.connect(self.change_transform)

    def setup_flipped_checkbox(self) -> None:
        self.ui.flipped_checkbox.setChecked(self.monitor.transform == Transform.flipped)
        self.ui.flipped_checkbox.clicked.connect(self.toggle_flipped)

    def change_transform(self) -> None:
        prev_transform = self.monitor.transform
        self.monitor.transform = Transform.reverse_map()[self.ui.transform_combobox.currentIndex()]
        if prev_transform.is_flipped:
            self.monitor.transform = self.monitor.transform.flipped_value()

        self.window().update_monitor_transform(self.monitor, prev_transform)

    def toggle_flipped(self) -> None:
        self.monitor.transform = self.monitor.transform.flipped_value()
        self.window().update_monitor_mirror(self.monitor.name)

    def setup_position_spinboxes(self) -> None:
        self.ui.position_x_spinbox.setValue(self.monitor.position.x)
        self.ui.position_y_spinbox.setValue(self.monitor.position.y)
        self.ui.position_x_spinbox.valueChanged.connect(self.change_position)
        self.ui.position_y_spinbox.valueChanged.connect(self.change_position)

    def set_position(self, x: int, y: int) -> None:
        self.ui.position_x_spinbox.setValue(x)
        self.ui.position_y_spinbox.setValue(y)
        self.monitor.position.x = x
        self.monitor.position.y = y

    def change_position(self) -> None:
        self.set_position(self.ui.position_x_spinbox.value(), self.ui.position_y_spinbox.value())
        self.window().update_monitor_positions(self.monitor)

    def setup_info_labels(self) -> None:
        self.ui.name_val_label.setText(self.monitor.name)
        self.ui.description_val_label.setText(self.monitor.description)

    def setup_scale_double_spinbox(self) -> None:
        self.ui.scale_double_spinbox.setValue(self.monitor.scale)
        self.ui.scale_double_spinbox.valueChanged.connect(self.change_scale)

    def setup_enabled_checkbox(self) -> None:
        self.ui.enabled_checkbox.setChecked(self.monitor.enabled)
        self.ui.enabled_checkbox.clicked.connect(self.toggle_disable)

    def toggle_disable(self) -> None:
        self.monitor.enabled = not self.monitor.enabled
        self.ui.enabled_checkbox.setChecked(not self.monitor.enabled)

    def setup_resolution_combobox(self) -> None:
        self.ui.resolution_combobox.addItems([str(mode) for mode in self.monitor.modes])
        self.ui.resolution_combobox.setCurrentIndex(
            self.ui.resolution_combobox.findText(str(self.monitor.active_mode)),
        )
        self.ui.resolution_combobox.activated.connect(self.change_resolution)

    def change_resolution(self, index: int) -> None:
        new_mode = self.monitor.modes[index]
        self.monitor.active_mode = new_mode
        self.window().update_monitor_resolution(self.monitor)

    def change_scale(self, value: float) -> None:
        self.monitor.scale = value
        self.window().update_monitor_scale(self.monitor)
