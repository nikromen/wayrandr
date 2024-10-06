# -*- coding: utf-8 -*-

################################################################################
## Form generated from reading UI file 'monitor_info_widget.ui'
##
## Created by: Qt User Interface Compiler version 6.7.2
##
## WARNING! All changes made in this file will be lost when recompiling UI file!
################################################################################

from PySide6.QtCore import (QCoreApplication, QDate, QDateTime, QLocale,
    QMetaObject, QObject, QPoint, QRect,
    QSize, QTime, QUrl, Qt)
from PySide6.QtGui import (QBrush, QColor, QConicalGradient, QCursor,
    QFont, QFontDatabase, QGradient, QIcon,
    QImage, QKeySequence, QLinearGradient, QPainter,
    QPalette, QPixmap, QRadialGradient, QTransform)
from PySide6.QtWidgets import (QApplication, QCheckBox, QComboBox, QDoubleSpinBox,
    QHBoxLayout, QLabel, QSizePolicy, QSpacerItem,
    QSpinBox, QVBoxLayout, QWidget)

class Ui_monitor_info_widget(object):
    def setupUi(self, monitor_info_widget):
        if not monitor_info_widget.objectName():
            monitor_info_widget.setObjectName(u"monitor_info_widget")
        monitor_info_widget.resize(400, 300)
        self.verticalLayoutWidget = QWidget(monitor_info_widget)
        self.verticalLayoutWidget.setObjectName(u"verticalLayoutWidget")
        self.verticalLayoutWidget.setGeometry(QRect(10, 10, 381, 281))
        self.main_vertical_layout = QVBoxLayout(self.verticalLayoutWidget)
        self.main_vertical_layout.setSpacing(10)
        self.main_vertical_layout.setObjectName(u"main_vertical_layout")
        self.main_vertical_layout.setContentsMargins(0, 0, 0, 0)
        self.monitor_checkboxs_layout = QHBoxLayout()
        self.monitor_checkboxs_layout.setObjectName(u"monitor_checkboxs_layout")
        self.enabled_checkbox = QCheckBox(self.verticalLayoutWidget)
        self.enabled_checkbox.setObjectName(u"enabled_checkbox")
        self.enabled_checkbox.setCursor(QCursor(Qt.CursorShape.PointingHandCursor))

        self.monitor_checkboxs_layout.addWidget(self.enabled_checkbox)

        self.flipped_checkbox = QCheckBox(self.verticalLayoutWidget)
        self.flipped_checkbox.setObjectName(u"flipped_checkbox")
        self.flipped_checkbox.setCursor(QCursor(Qt.CursorShape.PointingHandCursor))

        self.monitor_checkboxs_layout.addWidget(self.flipped_checkbox)

        self.button_top_spacer = QSpacerItem(40, 20, QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Minimum)

        self.monitor_checkboxs_layout.addItem(self.button_top_spacer)


        self.main_vertical_layout.addLayout(self.monitor_checkboxs_layout)

        self.name_horizontal_layout = QHBoxLayout()
        self.name_horizontal_layout.setObjectName(u"name_horizontal_layout")
        self.name_label = QLabel(self.verticalLayoutWidget)
        self.name_label.setObjectName(u"name_label")
        sizePolicy = QSizePolicy(QSizePolicy.Policy.Preferred, QSizePolicy.Policy.Preferred)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.name_label.sizePolicy().hasHeightForWidth())
        self.name_label.setSizePolicy(sizePolicy)
        self.name_label.setMinimumSize(QSize(0, 0))

        self.name_horizontal_layout.addWidget(self.name_label)

        self.name_val_label = QLabel(self.verticalLayoutWidget)
        self.name_val_label.setObjectName(u"name_val_label")
        sizePolicy.setHeightForWidth(self.name_val_label.sizePolicy().hasHeightForWidth())
        self.name_val_label.setSizePolicy(sizePolicy)

        self.name_horizontal_layout.addWidget(self.name_val_label)

        self.name_horizontal_layout.setStretch(0, 1)
        self.name_horizontal_layout.setStretch(1, 4)

        self.main_vertical_layout.addLayout(self.name_horizontal_layout)

        self.description_horizontal_layout = QHBoxLayout()
        self.description_horizontal_layout.setObjectName(u"description_horizontal_layout")
        self.description_label = QLabel(self.verticalLayoutWidget)
        self.description_label.setObjectName(u"description_label")

        self.description_horizontal_layout.addWidget(self.description_label)

        self.description_val_label = QLabel(self.verticalLayoutWidget)
        self.description_val_label.setObjectName(u"description_val_label")
        sizePolicy.setHeightForWidth(self.description_val_label.sizePolicy().hasHeightForWidth())
        self.description_val_label.setSizePolicy(sizePolicy)

        self.description_horizontal_layout.addWidget(self.description_val_label)

        self.description_horizontal_layout.setStretch(0, 1)
        self.description_horizontal_layout.setStretch(1, 4)

        self.main_vertical_layout.addLayout(self.description_horizontal_layout)

        self.resolution_horizontal_layout = QHBoxLayout()
        self.resolution_horizontal_layout.setObjectName(u"resolution_horizontal_layout")
        self.resolution_label = QLabel(self.verticalLayoutWidget)
        self.resolution_label.setObjectName(u"resolution_label")

        self.resolution_horizontal_layout.addWidget(self.resolution_label)

        self.resolution_combobox = QComboBox(self.verticalLayoutWidget)
        self.resolution_combobox.setObjectName(u"resolution_combobox")
        sizePolicy1 = QSizePolicy(QSizePolicy.Policy.Preferred, QSizePolicy.Policy.Fixed)
        sizePolicy1.setHorizontalStretch(0)
        sizePolicy1.setVerticalStretch(0)
        sizePolicy1.setHeightForWidth(self.resolution_combobox.sizePolicy().hasHeightForWidth())
        self.resolution_combobox.setSizePolicy(sizePolicy1)
        self.resolution_combobox.setCursor(QCursor(Qt.CursorShape.PointingHandCursor))

        self.resolution_horizontal_layout.addWidget(self.resolution_combobox)

        self.resolution_horizontal_layout.setStretch(0, 1)
        self.resolution_horizontal_layout.setStretch(1, 4)

        self.main_vertical_layout.addLayout(self.resolution_horizontal_layout)

        self.scale_horizontal_layout = QHBoxLayout()
        self.scale_horizontal_layout.setObjectName(u"scale_horizontal_layout")
        self.scale_label = QLabel(self.verticalLayoutWidget)
        self.scale_label.setObjectName(u"scale_label")

        self.scale_horizontal_layout.addWidget(self.scale_label)

        self.scale_double_spinbox = QDoubleSpinBox(self.verticalLayoutWidget)
        self.scale_double_spinbox.setObjectName(u"scale_double_spinbox")
        self.scale_double_spinbox.setCursor(QCursor(Qt.CursorShape.PointingHandCursor))
        self.scale_double_spinbox.setDecimals(6)
        self.scale_double_spinbox.setMaximum(9999.000000000000000)
        self.scale_double_spinbox.setSingleStep(0.100000000000000)

        self.scale_horizontal_layout.addWidget(self.scale_double_spinbox)

        self.scale_horizontal_layout.setStretch(0, 1)
        self.scale_horizontal_layout.setStretch(1, 4)

        self.main_vertical_layout.addLayout(self.scale_horizontal_layout)

        self.position_horizontal_layout = QHBoxLayout()
        self.position_horizontal_layout.setObjectName(u"position_horizontal_layout")
        self.position_label = QLabel(self.verticalLayoutWidget)
        self.position_label.setObjectName(u"position_label")

        self.position_horizontal_layout.addWidget(self.position_label)

        self.horizontal_position_spinbox_layout = QHBoxLayout()
        self.horizontal_position_spinbox_layout.setObjectName(u"horizontal_position_spinbox_layout")
        self.position_x_spinbox = QSpinBox(self.verticalLayoutWidget)
        self.position_x_spinbox.setObjectName(u"position_x_spinbox")
        self.position_x_spinbox.setCursor(QCursor(Qt.CursorShape.PointingHandCursor))
        self.position_x_spinbox.setMaximum(99999)

        self.horizontal_position_spinbox_layout.addWidget(self.position_x_spinbox)

        self.position_y_spinbox = QSpinBox(self.verticalLayoutWidget)
        self.position_y_spinbox.setObjectName(u"position_y_spinbox")
        self.position_y_spinbox.setCursor(QCursor(Qt.CursorShape.PointingHandCursor))
        self.position_y_spinbox.setMaximum(99999)

        self.horizontal_position_spinbox_layout.addWidget(self.position_y_spinbox)


        self.position_horizontal_layout.addLayout(self.horizontal_position_spinbox_layout)

        self.position_horizontal_layout.setStretch(0, 1)
        self.position_horizontal_layout.setStretch(1, 4)

        self.main_vertical_layout.addLayout(self.position_horizontal_layout)

        self.transform_horizontal_layout = QHBoxLayout()
        self.transform_horizontal_layout.setObjectName(u"transform_horizontal_layout")
        self.transform_label = QLabel(self.verticalLayoutWidget)
        self.transform_label.setObjectName(u"transform_label")

        self.transform_horizontal_layout.addWidget(self.transform_label)

        self.transform_combobox = QComboBox(self.verticalLayoutWidget)
        self.transform_combobox.setObjectName(u"transform_combobox")
        self.transform_combobox.setCursor(QCursor(Qt.CursorShape.PointingHandCursor))

        self.transform_horizontal_layout.addWidget(self.transform_combobox)

        self.transform_horizontal_layout.setStretch(0, 1)
        self.transform_horizontal_layout.setStretch(1, 4)

        self.main_vertical_layout.addLayout(self.transform_horizontal_layout)

        self.verticalSpacer = QSpacerItem(20, 40, QSizePolicy.Policy.Minimum, QSizePolicy.Policy.Expanding)

        self.main_vertical_layout.addItem(self.verticalSpacer)


        self.retranslateUi(monitor_info_widget)

        QMetaObject.connectSlotsByName(monitor_info_widget)
    # setupUi

    def retranslateUi(self, monitor_info_widget):
        monitor_info_widget.setWindowTitle(QCoreApplication.translate("monitor_info_widget", u"Form", None))
        self.enabled_checkbox.setText(QCoreApplication.translate("monitor_info_widget", u"Enabled", None))
        self.flipped_checkbox.setText(QCoreApplication.translate("monitor_info_widget", u"Flipped", None))
        self.name_label.setText(QCoreApplication.translate("monitor_info_widget", u"Name", None))
        self.name_val_label.setText(QCoreApplication.translate("monitor_info_widget", u"Unknown", None))
        self.description_label.setText(QCoreApplication.translate("monitor_info_widget", u"Description", None))
        self.description_val_label.setText(QCoreApplication.translate("monitor_info_widget", u"Unknown", None))
        self.resolution_label.setText(QCoreApplication.translate("monitor_info_widget", u"Resolution", None))
        self.scale_label.setText(QCoreApplication.translate("monitor_info_widget", u"Scale", None))
        self.position_label.setText(QCoreApplication.translate("monitor_info_widget", u"Position", None))
        self.transform_label.setText(QCoreApplication.translate("monitor_info_widget", u"Transform", None))
    # retranslateUi

