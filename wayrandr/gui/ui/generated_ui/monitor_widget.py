# -*- coding: utf-8 -*-

################################################################################
## Form generated from reading UI file 'monitor_widget.ui'
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
from PySide6.QtWidgets import (QApplication, QFrame, QGridLayout, QHBoxLayout,
    QLabel, QSizePolicy, QSpacerItem, QVBoxLayout,
    QWidget)

class Ui_monitor_widget(object):
    def setupUi(self, monitor_widget):
        if not monitor_widget.objectName():
            monitor_widget.setObjectName(u"monitor_widget")
        monitor_widget.resize(400, 300)
        monitor_widget.setCursor(QCursor(Qt.CursorShape.OpenHandCursor))
        monitor_widget.setFrameShape(QFrame.Shape.Box)
        self.gridLayout = QGridLayout(monitor_widget)
        self.gridLayout.setObjectName(u"gridLayout")
        self.vertical_layout = QVBoxLayout()
        self.vertical_layout.setObjectName(u"vertical_layout")
        self.horizontal_layout = QHBoxLayout()
        self.horizontal_layout.setObjectName(u"horizontal_layout")
        self.monitor_name_label = QLabel(monitor_widget)
        self.monitor_name_label.setObjectName(u"monitor_name_label")
        font = QFont()
        font.setPointSize(14)
        self.monitor_name_label.setFont(font)
        self.monitor_name_label.setStyleSheet(u"background-color: black;\n"
"color: white;")

        self.horizontal_layout.addWidget(self.monitor_name_label)

        self.horizontal_spacer = QSpacerItem(40, 20, QSizePolicy.Policy.Expanding, QSizePolicy.Policy.Minimum)

        self.horizontal_layout.addItem(self.horizontal_spacer)


        self.vertical_layout.addLayout(self.horizontal_layout)

        self.vertical_spacer = QSpacerItem(20, 40, QSizePolicy.Policy.Minimum, QSizePolicy.Policy.Expanding)

        self.vertical_layout.addItem(self.vertical_spacer)


        self.gridLayout.addLayout(self.vertical_layout, 0, 0, 1, 1)


        self.retranslateUi(monitor_widget)

        QMetaObject.connectSlotsByName(monitor_widget)
    # setupUi

    def retranslateUi(self, monitor_widget):
        self.monitor_name_label.setText(QCoreApplication.translate("monitor_widget", u"Unknown", None))
        pass
    # retranslateUi

