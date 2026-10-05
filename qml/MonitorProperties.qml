import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import com.nikromen.wayrandr 1.0

ScrollView {
    property var monitor

    implicitHeight: mainLayout.implicitHeight

    Frame {
        id: mainFrame
        width: parent.width

        ColumnLayout {
            id: mainLayout
            width: parent.width

            OutputToggles {
                item: monitor
                showAdaptiveSync: true
                adaptiveSyncEnabled: monitor.hasSettings
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
            }

            GridLayout {
                visible: monitor.hasSettings
                columns: 2
                columnSpacing: 10
                Layout.fillWidth: true
                enabled: monitor.hasSettings

                Label { text: qsTr("Resolution:") }
                RowLayout {
                    Layout.fillWidth: true
                    ComboBox {
                        id: resolutionCombo
                        objectName: "resolutionCombo"
                        Layout.fillWidth: true
                        model: monitor.resolutions
                        currentIndex: {
                            if (!monitor.hasSettings || count === 0) {
                                return -1
                            }
                            return monitor.activeResolutionIndex
                        }
                        onActivated: function(index) {
                            monitor.activeResolutionIndex = index
                        }
                    }
                    Button {
                        id: preferredModeButton
                        text: qsTr("Preferred")
                        ToolTip.text: qsTr("Set preferred/native mode")
                        onClicked: monitor.activate_preferred_mode()
                    }
                }

                OutputSettingsForm {
                    Layout.columnSpan: 2
                    item: monitor
                    settingsEnabled: monitor.hasSettings
                }

                Label { text: qsTr("Position:") }
                RowLayout {
                    Layout.fillWidth: true
                    SpinBox {
                        id: posXSpinBox
                        from: 0
                        to: 1000000
                        value: monitor.positionX
                        onValueChanged: monitor.positionX = value
                        editable: true
                    }
                    Label { text: "x" }
                    SpinBox {
                        id: posYSpinBox
                        from: 0
                        to: 1000000
                        value: monitor.positionY
                        onValueChanged: monitor.positionY = value
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
