import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import io.github.nikromen.wayrandr 1.0

ApplicationWindow {
    id: window
    visible: true

    // Dynamic sizing - window will adapt to content
    width: Screen.desktopAvailableWidth * 0.8
    height: Screen.desktopAvailableHeight * 0.8
    minimumWidth: 640
    minimumHeight: 480

    title: qsTr("wayrandr")

    MainWindow {
        anchors.fill: parent
    }
}
