import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property alias name: monitorNameLabel.text
    property int monitorResolutionWidth: 1920
    property int monitorResolutionHeight: 1080
    property real displayScale: 0.1

    property string monitorIdentifier: "Unknown"
    property alias imageSource: screenPreviewImage.source

    signal pressed(var mouse)
    signal positionChanged(var mouse)
    signal released(var mouse)

    width: monitorResolutionWidth * displayScale
    height: monitorResolutionHeight * displayScale
    color: "black"
    border.width: 1
    border.color: "skyblue"

    radius: 6
    clip: true

    Image {
        id: screenPreviewImage
        anchors.fill: parent
        cache: false
        fillMode: Image.PreserveAspectCrop
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        Label {
            id: monitorNameLabel
            text: root.monitorIdentifier
            font.pointSize: 10
            color: "white"
            padding: 4
            background: Rectangle { color: "rgba(0, 0, 0, 0.7)" }
            Layout.fillWidth: true
        }
        Item { Layout.fillHeight: true }
    }

    MouseArea {
        id: dragArea
        anchors.fill: parent
        cursorShape: Qt.OpenHandCursor

        onPressed: {
            cursorShape = Qt.ClosedHandCursor
            root.pressed(mouse)
        }

        onPositionChanged: {
            if (pressed) {
                root.positionChanged(mouse)
            }
        }

        onReleased: {
            cursorShape = Qt.OpenHandCursor
            root.released(mouse)
        }
    }
}
