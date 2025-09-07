import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    property var monitor

    implicitHeight: mainLayout.implicitHeight

    Frame {
        id: mainFrame
        width: parent.width

        ColumnLayout {
            id: mainLayout
            width: parent.width

            RowLayout {
                Switch {
                    id: enabledSwitch
                    text: qsTr("Enabled")
                    onCheckedChanged: monitor.enabled = checked
                }

                Switch {
                    id: flippedSwitch
                    text: qsTr("Flipped")
                    onCheckedChanged: monitor.flipped = checked
                }

                Switch {
                    id: adaptiveSyncSwitch
                    text: qsTr("Adaptive Sync")
                    onCheckedChanged: monitor.adaptiveSync = checked
                }

                Item { Layout.fillWidth: true }
            }

            GridLayout {
                columns: 2
                columnSpacing: 10
                Layout.fillWidth: true

                Label { text: qsTr("Name:") }
                Label { id: nameValue; text: monitor.name; Layout.fillWidth: true }

                Label { text: qsTr("Description:") }
                Label {
                    id: descriptionValue
                    text: monitor.description
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }

                Label { text: qsTr("Resolution:") }
                RowLayout {
                    Layout.fillWidth: true
                    ComboBox {
                        id: resolutionCombo
                        Layout.fillWidth: true
                        currentIndex: monitor.activeResolutionIndex
                        onCurrentIndexChanged: monitor.activeResolutionIndex = currentIndex
                    }
                    Button {
                        id: preferredModeButton
                        text: qsTr("Preferred")
                        ToolTip.text: qsTr("Set preferred/native mode")
                    }
                }

                Label { text: qsTr("Scale:") }
                SpinBox {
                    id: scaleSpinBox
                    Layout.fillWidth: true
                    from: 0.1
                    stepSize: 0.1
                    value: monitor.scale
                    onValueChanged: monitor.scale = value
                }

                Label { text: qsTr("Position:") }
                RowLayout {
                    Layout.fillWidth: true
                    SpinBox { id: posXSpinBox; value: monitor.positionX; onValueChanged: monitor.positionX = value }
                    Label { text: "x" }
                    SpinBox { id: posYSpinBox; value: monitor.positionY; onValueChanged: monitor.positionY = value }
                }

                Label { text: qsTr("Transform:") }
                ComboBox {
                    id: transformCombo
                    Layout.fillWidth: true
                    model: monitor.getTransformList
                    currentIndex: monitor.transform
                    onCurrentIndexChanged: monitor.transform = currentIndex
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
