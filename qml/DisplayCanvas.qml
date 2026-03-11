import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root

    implicitWidth: 0
    implicitHeight: 0

    required property var controller
    property var canvasModel
    // UI preview zoom only — unrelated to wlr-randr output scale
    property real displayScale: 0.1
    property var blockName: null
    property var blockImageSource: null
    property var blockVisible: null
    property var onBlockPressed: null

    readonly property bool hasBlockName: typeof blockName === "function"
    readonly property bool hasBlockImageSource: typeof blockImageSource === "function"
    readonly property bool hasBlockVisible: typeof blockVisible === "function"
    readonly property bool hasOnBlockPressed: typeof onBlockPressed === "function"

    ColumnLayout {
        id: columnLayout
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 8
            Layout.rightMargin: 8
            Layout.topMargin: 4
            Layout.bottomMargin: 4

            BackendSwitcher {
                controller: root.controller
            }

            Item { Layout.fillWidth: true }

            ToolButton {
                text: qsTr("Reset layout")
                flat: true
                ToolTip.text: qsTr("Arrange outputs side by side from the top-left corner")
                onClicked: controller.reset_canvas_layout(displayCanvas.width, root.displayScale)
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: palette.mid
        }

        ScrollView {
            id: canvasScroller
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 0
            clip: true

            Item {
                id: displayCanvas
                width: canvasScroller.availableWidth
                height: canvasScroller.availableHeight
                clip: true

                Repeater {
                    model: root.canvasModel

                    MonitorBlock {
                        name: {
                            if (root.hasBlockName) {
                                return root.blockName(modelData, index)
                            }
                            return ""
                        }
                        displayScale: root.displayScale
                        monitorResolutionWidth: modelData.layoutWidth
                        monitorResolutionHeight: modelData.layoutHeight
                        monitorTransform: modelData.transform
                        imageSource: {
                            if (root.hasBlockImageSource) {
                                return root.blockImageSource(modelData, index)
                            }
                            return ""
                        }
                        visible: {
                            if (root.hasBlockVisible) {
                                return root.blockVisible(modelData, index)
                            }
                            return modelData.enabled
                        }

                        x: modelData.positionX * root.displayScale
                        y: modelData.positionY * root.displayScale

                        onPressed: function(mouse) {
                            var canvasPos = mapToItem(displayCanvas, mouse.x, mouse.y)
                            modelData.start_drag(canvasPos.x, canvasPos.y)
                            if (root.hasOnBlockPressed) {
                                root.onBlockPressed(modelData, index)
                            }
                        }

                        onPositionChanged: function(mouse) {
                            var canvasPos = mapToItem(displayCanvas, mouse.x, mouse.y)
                            modelData.update_drag(
                                canvasPos.x,
                                canvasPos.y,
                                root.displayScale,
                                controller,
                                displayCanvas.width,
                                displayCanvas.height
                            )
                        }
                    }
                }
            }
        }
    }
}
