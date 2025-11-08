import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property alias name: monitorNameLabel.text
    property int monitorResolutionWidth: 1920
    property int monitorResolutionHeight: 1080
    // TODO: nastavitelny display scale
    property real displayScale: 0.1

    property string monitorIdentifier: "Unknown"
    property string imageSource: ""

    signal pressed(var mouse)
    signal positionChanged(var mouse)
    signal released(var mouse)

    width: monitorResolutionWidth * displayScale
    height: monitorResolutionHeight * displayScale
    color: "black"

    clip: true

    // TODO: why flicker when updating source with single image?
    // this is a hack to avoid the black flicker
    Image {
        id: screenPreviewImage1
        anchors.fill: parent
        cache: false
        asynchronous: true
        fillMode: Image.PreserveAspectCrop
        visible: !screenPreviewImage2.visible
    }

    Image {
        id: screenPreviewImage2
        anchors.fill: parent
        cache: false
        asynchronous: true
        fillMode: Image.PreserveAspectCrop
        visible: false
    }

    onImageSourceChanged: {
        if (imageSource === "") {
            return
        }

        if (screenPreviewImage1.visible) {
            screenPreviewImage2.source = imageSource
        } else {
            screenPreviewImage1.source = imageSource
        }
    }

    Connections {
        target: screenPreviewImage1
        function onStatusChanged() {
            if (screenPreviewImage1.status === Image.Ready && !screenPreviewImage1.visible) {
                screenPreviewImage1.visible = true
                screenPreviewImage2.visible = false
            }
        }
    }

    Connections {
        target: screenPreviewImage2
        function onStatusChanged() {
            if (screenPreviewImage2.status === Image.Ready && !screenPreviewImage2.visible) {
                screenPreviewImage2.visible = true
                screenPreviewImage1.visible = false
            }
        }
    }

    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: monitorNameLabel.height
        color: Qt.rgba(0, 0, 0, 0.7)

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: parent.height
            color: parent.color
        }

        Label {
            id: monitorNameLabel
            text: root.monitorIdentifier
            font.pointSize: 10
            color: "white"
            padding: 4
            anchors.centerIn: parent
        }
    }

    MouseArea {
        id: dragArea
        anchors.fill: parent
        cursorShape: Qt.OpenHandCursor

        onPressed: function(mouse) {
            cursorShape = Qt.ClosedHandCursor
            root.pressed(mouse)
        }

        onPositionChanged: function(mouse) {
            if (pressed) {
                root.positionChanged(mouse)
            }
        }

        onReleased: function(mouse) {
            cursorShape = Qt.OpenHandCursor
            root.released(mouse)
        }
    }
}
