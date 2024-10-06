# -*- coding: utf-8 -*-

################################################################################
## Form generated from reading UI file 'main_window.ui'
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
from PySide6.QtWidgets import (QApplication, QGridLayout, QMainWindow, QPushButton,
    QSizePolicy, QTabWidget, QWidget)

class Ui_main_window(object):
    def setupUi(self, main_window):
        if not main_window.objectName():
            main_window.setObjectName(u"main_window")
        main_window.resize(800, 600)
        self.central_widget = QWidget(main_window)
        self.central_widget.setObjectName(u"central_widget")
        self.gridLayout_2 = QGridLayout(self.central_widget)
        self.gridLayout_2.setObjectName(u"gridLayout_2")
        self.main_grid_layout = QGridLayout()
        self.main_grid_layout.setObjectName(u"main_grid_layout")
        self.monitor_tab_widget = QTabWidget(self.central_widget)
        self.monitor_tab_widget.setObjectName(u"monitor_tab_widget")
        sizePolicy = QSizePolicy(QSizePolicy.Policy.Fixed, QSizePolicy.Policy.Expanding)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.monitor_tab_widget.sizePolicy().hasHeightForWidth())
        self.monitor_tab_widget.setSizePolicy(sizePolicy)
        self.monitor_tab_widget.setMinimumSize(QSize(430, 0))

        self.main_grid_layout.addWidget(self.monitor_tab_widget, 0, 1, 1, 1)

        self.save_button = QPushButton(self.central_widget)
        self.save_button.setObjectName(u"save_button")
        sizePolicy1 = QSizePolicy(QSizePolicy.Policy.Minimum, QSizePolicy.Policy.Expanding)
        sizePolicy1.setHorizontalStretch(0)
        sizePolicy1.setVerticalStretch(0)
        sizePolicy1.setHeightForWidth(self.save_button.sizePolicy().hasHeightForWidth())
        self.save_button.setSizePolicy(sizePolicy1)
        self.save_button.setCursor(QCursor(Qt.CursorShape.PointingHandCursor))

        self.main_grid_layout.addWidget(self.save_button, 1, 0, 1, 2)

        self.monitors_area_widget = QWidget(self.central_widget)
        self.monitors_area_widget.setObjectName(u"monitors_area_widget")

        self.main_grid_layout.addWidget(self.monitors_area_widget, 0, 0, 1, 1)

        self.main_grid_layout.setRowStretch(0, 24)

        self.gridLayout_2.addLayout(self.main_grid_layout, 0, 0, 1, 1)

        main_window.setCentralWidget(self.central_widget)

        self.retranslateUi(main_window)

        self.monitor_tab_widget.setCurrentIndex(-1)


        QMetaObject.connectSlotsByName(main_window)
    # setupUi

    def retranslateUi(self, main_window):
        main_window.setWindowTitle(QCoreApplication.translate("main_window", u"Monitor Configuration", None))
        self.save_button.setText(QCoreApplication.translate("main_window", u"Save", None))
    # retranslateUi

