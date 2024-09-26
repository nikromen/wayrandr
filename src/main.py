from src.ui import MainWindow

import sys
from PyQt5.QtWidgets import QApplication


def run() -> None:
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec_())


if __name__ == "__main__":
    run()
