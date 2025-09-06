// DisplayInfoOverlay.qml
import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: root

    property alias outputName: outputNameLabel.text
    property alias monitorName: monitorNameLabel.text

    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.WindowDoesNotAcceptFocus
    color: "transparent"

    width: contentRectangle.implicitWidth
    height: contentRectangle.implicitHeight

    Rectangle {
        id: contentRectangle
        anchors.fill: parent

        color: "rgba(30, 30, 30, 0.85)"
        radius: 8

        implicitWidth: mainLayout.implicitWidth
        implicitHeight: mainLayout.implicitHeight

        ColumnLayout {
            id: mainLayout
            anchors.fill: parent
            anchors.margins: 15

            Label {
                id: outputNameLabel
                text: "DP-8"
                color: "white"
                font.pointSize: 16
                font.bold: true
            }

            Label {
                id: monitorNameLabel
                text: "Dell UltraSharp U2721DE"
                color: "#cccccc"
                font.pointSize: 10
            }
        }
    }
}
