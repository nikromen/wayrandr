import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    visible: true

    width: Screen.desktopAvailableWidth * 0.8
    height: Screen.desktopAvailableHeight * 0.8
    minimumWidth: 640
    minimumHeight: 480

    title: qsTr("wayrandr")

    BackendStack {
        anchors.fill: parent
        controller: mainWindow
    }
}
