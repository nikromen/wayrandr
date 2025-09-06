import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    implicitHeight: mainLayout.implicitHeight

    Frame {
        id: mainFrame
        width: parent.width

        ColumnLayout {
            id: mainLayout
            width: parent.width

            RowLayout {
                Switch { id: enabledSwitch; text: qsTr("Enabled") }

                Switch { id: flippedSwitch; text: qsTr("Flipped") }

                Switch { id: adaptiveSyncSwitch; text: qsTr("Adaptive Sync") }

                Item { Layout.fillWidth: true }
            }

            GridLayout {
                columns: 2
                columnSpacing: 10
                Layout.fillWidth: true

                Label { text: qsTr("Name:") }
                Label { id: nameValue; text: "Unknown"; Layout.fillWidth: true }

                Label { text: qsTr("Description:") }
                Label { id: descriptionValue; text: "Unknown"; Layout.fillWidth: true; wrapMode: Text.WordWrap }

                Label { text: qsTr("Resolution:") }
                RowLayout {
                    Layout.fillWidth: true
                    ComboBox { id: resolutionCombo; Layout.fillWidth: true }
                    Button {
                        id: preferredModeButton
                        text: qsTr("Preferred")
                        ToolTip.text: qsTr("Set preferred/native mode")
                    }
                }

                Label { text: qsTr("Scale:") }
                TextField {
                    id: scaleTextField
                    Layout.fillWidth: true
                    validator: DoubleValidator { bottom: 0.1 }
                }

                Label { text: qsTr("Position:") }
                RowLayout {
                    Layout.fillWidth: true
                    SpinBox { id: posXSpinBox; value: 0; Layout.fillWidth: true }
                    Label { text: "x" }
                    SpinBox { id: posYSpinBox; value: 0; Layout.fillWidth: true }
                }

                Label { text: qsTr("Transform:") }
                ComboBox { id: transformCombo; Layout.fillWidth: true }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
