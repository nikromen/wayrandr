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
    property string imageSource: ""
    property string monitorTransform: "normal"

    signal pressed(var mouse)
    signal positionChanged(var mouse)
    signal released(var mouse)

    readonly property real previewRotation: {
        switch (monitorTransform) {
        case "90":
        case "flipped-90":
            return 90
        case "180":
        case "flipped-180":
            return 180
        case "270":
        case "flipped-270":
            return 270
        default:
            return 0
        }
    }

    readonly property real previewScaleX: monitorTransform.indexOf("flipped") >= 0 ? -1 : 1

    width: monitorResolutionWidth * displayScale
    height: monitorResolutionHeight * displayScale
    color: "black"

    clip: true

    Item {
        id: previewContainer
        anchors.fill: parent

        transform: [
            Scale {
                xScale: root.previewScaleX
                origin.x: previewContainer.width / 2
                origin.y: previewContainer.height / 2
            },
            Rotation {
                angle: root.previewRotation
                origin.x: previewContainer.width / 2
                origin.y: previewContainer.height / 2
            }
        ]

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
        z: 1

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
        z: 2

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
