import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    SplitView {
        id: mainSplitView
        anchors.fill: parent
        orientation: Qt.Horizontal

        ColumnLayout {
            id: leftPanel

            Layout.minimumWidth: 320
            Layout.preferredWidth: 350
            Layout.maximumWidth: 500

            ButtonGroup {
                id: displayManagerGroup
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 20

                RadioButton {
                    text: "wlr-randr"
                    checked: true
                    ButtonGroup.group: displayManagerGroup
                }

                RadioButton {
                    text: "auto-wlr-randr"
                    ButtonGroup.group: displayManagerGroup
                }

                RadioButton {
                    text: "kanshi"
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

            StackLayout {
                id: monitorStackLayout
                Layout.fillWidth: true
                Layout.fillHeight: true

                currentIndex: monitorTabBar.currentIndex
            }

            RowLayout {
                id: actionButtons
                spacing: 10
                Layout.fillWidth: true

                Button {
                    id: applyButton
                    text: qsTr("Apply")
                    Layout.fillWidth: true
                    onClicked: mainWindow.apply()
                }
                Button {
                    id: saveButton
                    text: qsTr("Save")
                    highlighted: true
                    Layout.fillWidth: true
                    onClicked: mainWindow.save()
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
                }
            }
        }
    }
}
