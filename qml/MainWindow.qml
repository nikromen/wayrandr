import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import io.github.nikromen.wayrandr 1.0

Rectangle {
    id: root

    readonly property real displayScale: 0.1

    SplitView {
        id: mainSplitView
        anchors.fill: parent
        orientation: Qt.Horizontal

        ColumnLayout {
            id: leftPanel

            ButtonGroup {
                id: displayManagerGroup
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 20

                // TODO: support for kanshi and auto-wlr-randr
                RadioButton {
                    text: "wlr-randr"
                    checked: true
                    ButtonGroup.group: displayManagerGroup
                }
            }

            TabBar {
                id: monitorTabBar
                Layout.fillWidth: true

                Repeater {
                    model: mainWindow.monitors
                    TabButton {
                        text: modelData.name
                    }
                }
            }

            Connections {
                target: mainWindow
                function onSwitch_to_monitor_tab(index) {
                    monitorTabBar.currentIndex = index
                }
            }

            StackLayout {
                id: monitorStackLayout
                Layout.fillWidth: true
                Layout.fillHeight: true

                currentIndex: monitorTabBar.currentIndex

                Repeater {
                    model: mainWindow.monitors

                    MonitorProperties {
                        monitor: modelData
                        width: parent.width
                    }
                }
            }

            RowLayout {
                id: actionButtons
                spacing: 10
                Layout.fillWidth: true

                Button {
                    id: applyButton
                    text: qsTr("Apply")
                    highlighted: true
                    Layout.fillWidth: true
                    visible: !mainWindow.confirmationPending
                    onClicked: mainWindow.apply()
                }

                Button {
                    id: confirmButton
                    text: qsTr("Confirm")
                    highlighted: true
                    Layout.fillWidth: true
                    visible: mainWindow.confirmationPending
                    onClicked: mainWindow.confirm_apply()
                }

                Button {
                    id: cancelButton
                    text: qsTr("Cancel")
                    Layout.fillWidth: true
                    visible: mainWindow.confirmationPending
                    onClicked: mainWindow.cancel_apply()
                }

                Label {
                    visible: mainWindow.confirmationPending
                    text: qsTr("Reverting in %1 s").arg(mainWindow.confirmationSecondsLeft)
                }
            }
        }

        Rectangle {
            id: rightPanel
            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 8
                    Layout.rightMargin: 8
                    Layout.topMargin: 4
                    Layout.bottomMargin: 4

                    Item { Layout.fillWidth: true }

                    ToolButton {
                        text: qsTr("Reset layout")
                        flat: true
                        ToolTip.text: qsTr("Arrange monitors side by side from the top-left corner")
                        onClicked: mainWindow.reset_canvas_layout(displayCanvas.width, root.displayScale)
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

                    Item {
                        id: displayCanvas
                        width: canvasScroller.availableWidth
                        height: canvasScroller.availableHeight
                        clip: true

                        Repeater {
                            model: mainWindow.monitors

                            MonitorBlock {
                                id: monitorBlock

                                property var monitorBlockModel: mainWindow.get_monitor_block(modelData.name)

                                name: modelData.name
                                displayScale: root.displayScale
                                monitorResolutionWidth: modelData.layoutWidth
                                monitorResolutionHeight: modelData.layoutHeight
                                monitorTransform: modelData.transform
                                imageSource: monitorBlockModel ? monitorBlockModel.screenImage : ""
                                visible: modelData.enabled

                                x: modelData.positionX * root.displayScale
                                y: modelData.positionY * root.displayScale

                                onPressed: function(mouse) {
                                    var canvasPos = mapToItem(displayCanvas, mouse.x, mouse.y)
                                    modelData.start_drag(canvasPos.x, canvasPos.y)

                                    var monitorIndex = mainWindow.get_monitor_index(modelData)
                                    if (monitorIndex >= 0) {
                                        mainWindow.switch_to_monitor_tab(monitorIndex)
                                    }
                                }

                                onPositionChanged: function(mouse) {
                                    var canvasPos = mapToItem(displayCanvas, mouse.x, mouse.y)
                                    modelData.update_drag(
                                        canvasPos.x,
                                        canvasPos.y,
                                        root.displayScale,
                                        mainWindow,
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
    }
}
