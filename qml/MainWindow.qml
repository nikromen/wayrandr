import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import io.github.nikromen.wayrandr 1.0

Rectangle {
    id: root

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
                    onClicked: mainWindow.apply()
                }
            }
        }

        Rectangle {
            id: rightPanel
            Layout.fillWidth: true

            ScrollView {
                id: canvasScroller
                anchors.fill: parent

                Item {
                    id: displayCanvas
                    width: canvasScroller.availableWidth
                    height: canvasScroller.availableHeight

                    Repeater {
                        model: mainWindow.monitors

                        MonitorBlock {
                            id: monitorBlock

                            property var monitorBlockModel: mainWindow.get_monitor_block(modelData.name)

                            name: modelData.name
                            monitorResolutionWidth: modelData.resolutionWidth
                            monitorResolutionHeight: modelData.resolutionHeight
                            imageSource: monitorBlockModel ? monitorBlockModel.screenImage : ""
                            visible: modelData.enabled

                            x: modelData.positionX * displayScale
                            y: modelData.positionY * displayScale

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
                                modelData.update_drag(canvasPos.x, canvasPos.y, displayScale, mainWindow)
                            }
                        }
                    }
                }
            }
        }
    }
}
