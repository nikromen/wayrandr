from PySide6.QtCore import Qt
from PySide6.QtWidgets import (
    QButtonGroup,
    QCheckBox,
    QComboBox,
    QDoubleSpinBox,
    QFrame,
    QLabel,
    QRadioButton,
    QSizePolicy,
    QSpinBox,
)

from src.monitors import Monitor, Transform


class MonitorInfoWidget(QFrame):
    def __init__(self, monitor: Monitor) -> None:
        super().__init__()
        self._curr_move_y = -1

        self.monitor = monitor

        self.setup_info_labels()

        self.resolution_combobox = QComboBox(self)
        self.setup_resolution_combobox()

        self.scale = QDoubleSpinBox(self)
        self.setup_scale()

        self.checkbox = QCheckBox("Enabled", self)
        self.setup_checkbox()

        self.position_x = QSpinBox(self)
        self.position_y = QSpinBox(self)
        self.setup_position_fields()

        self.transform_button = QButtonGroup(self)
        self.setup_transform_buttons()

        self.flipped_button = QCheckBox("Flipped", self)
        self.setup_flipped_button()

    def setup_transform_buttons(self) -> None:
        transform_label = QLabel("Transform:", self)
        transform_label.move(10, self._get_next_move_y())

        self.transform_button.addButton(QRadioButton("Don't rotate", self))
        self.transform_button.addButton(QRadioButton("Rotate 90°", self))
        self.transform_button.addButton(QRadioButton("Rotate 180°", self))
        self.transform_button.addButton(QRadioButton("Rotate 270°", self))

        self.transform_button.buttonClicked.connect(self.change_transform)
        self.transform_button.id = lambda button: self.transform_button.buttons().index(button)
        self.transform_button.buttons()[self.monitor.transform.value_index()].setChecked(True)

        for button in self.transform_button.buttons():
            if button.text() == "Don't rotate":
                button.move(100, self._curr_move_y)
            else:
                button.move(100, self._get_next_move_y())

            button.setCursor(Qt.CursorShape.PointingHandCursor)
            button.setSizePolicy(QSizePolicy(QSizePolicy.Minimum, QSizePolicy.Minimum))

    def setup_flipped_button(self) -> None:
        self.flipped_button.move(10, self._get_next_move_y())
        self.flipped_button.setChecked(self.monitor.transform == Transform.flipped)
        self.flipped_button.clicked.connect(self.toggle_flipped)
        self.flipped_button.setCursor(Qt.CursorShape.PointingHandCursor)

    def change_transform(self, button: QRadioButton) -> None:
        prev_transform = self.monitor.transform
        self.monitor.transform = Transform.reverse_map()[self.transform_button.id(button)]
        self.window().update_monitor_transform(self.monitor, prev_transform)

    def toggle_flipped(self) -> None:
        self.monitor.transform = self.monitor.transform.flipped_value()
        self.window().update_monitor_mirror(self.monitor.name)

    def _get_next_move_y(self, offset: int = 40) -> int:
        if self._curr_move_y == -1:
            self._curr_move_y = 10
        else:
            self._curr_move_y += offset

        return self._curr_move_y

    def setup_position_fields(self) -> None:
        position_label = QLabel("Position:", self)
        position_label.move(10, self._get_next_move_y())
        self.position_x.setRange(0, 99999)
        self.position_y.setRange(0, 99999)
        self.position_x.move(100, self._curr_move_y)
        self.position_y.move(180, self._curr_move_y)
        self.position_x.setValue(self.monitor.position.x)
        self.position_y.setValue(self.monitor.position.y)
        self.position_x.setCursor(Qt.CursorShape.PointingHandCursor)
        self.position_y.setCursor(Qt.CursorShape.PointingHandCursor)
        self.position_x.valueChanged.connect(self.change_position)
        self.position_y.valueChanged.connect(self.change_position)

    def set_position(self, x: int, y: int) -> None:
        self.position_x.setValue(x)
        self.position_y.setValue(y)
        self.monitor.position.x = x
        self.monitor.position.y = y

    def change_position(self) -> None:
        self.set_position(self.position_x.value(), self.position_y.value())
        self.window().update_monitor_positions(self.monitor)

    def setup_info_labels(self) -> None:
        name_label = QLabel(f"Name: {self.monitor.name}", self)
        name_label.move(10, self._get_next_move_y())

        description_label = QLabel(f"Description: {self.monitor.description}", self)
        description_label.move(10, self._get_next_move_y())

    def setup_scale(self) -> None:
        scale_label = QLabel("Scale:", self)
        scale_label.move(10, self._get_next_move_y())
        self.scale.move(100, self._curr_move_y)
        self.scale.setDecimals(6)
        self.scale.setRange(0.1, 9999)
        self.scale.setSingleStep(0.1)
        self.scale.setValue(self.monitor.scale)
        self.scale.valueChanged.connect(self.change_scale)
        self.scale.setCursor(Qt.CursorShape.PointingHandCursor)

    def setup_checkbox(self) -> None:
        self.checkbox.move(10, self._get_next_move_y())
        self.checkbox.setChecked(self.monitor.enabled)
        self.checkbox.clicked.connect(self.toggle_disable)
        self.checkbox.setCursor(Qt.CursorShape.PointingHandCursor)

    def toggle_disable(self) -> None:
        self.monitor.enabled = not self.monitor.enabled
        self.checkbox.setChecked(not self.monitor.enabled)

    def setup_resolution_combobox(self) -> None:
        resolution_label = QLabel("Resolution:", self)
        resolution_label.move(10, self._get_next_move_y())
        self.resolution_combobox.move(100, self._curr_move_y)
        self.resolution_combobox.addItems([str(mode) for mode in self.monitor.modes])
        self.resolution_combobox.setCurrentIndex(
            self.resolution_combobox.findText(str(self.monitor.active_mode)),
        )
        self.resolution_combobox.setCursor(Qt.CursorShape.PointingHandCursor)
        self.resolution_combobox.activated.connect(self.change_resolution)
        self.resolution_combobox.setSizeAdjustPolicy(QComboBox.SizeAdjustPolicy.AdjustToContents)
        self.resolution_combobox.setSizePolicy(
            QSizePolicy(QSizePolicy.Minimum, QSizePolicy.Minimum),
        )

    def change_resolution(self, index: int) -> None:
        new_mode = self.monitor.modes[index]
        self.monitor.active_mode = new_mode
        self.resize(*self.monitor.active_mode.scaled_resolution())
        self.window().update_monitor_resolution(self.monitor)

    def change_scale(self, value: float) -> None:
        self.monitor.scale = value
        self.window().update_monitor_scale(self.monitor)
