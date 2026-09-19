import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import com.nikromen.wayrandr 1.0

ScrollView {
    id: root
    objectName: "monitorProperties"

    property var monitor
    readonly property bool hasMonitor: !!monitor

    // Backend positions and QML int properties use signed 32-bit coordinates.
    readonly property int minimumPosition: -2147483648
    readonly property int maximumPosition: 2147483647

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
                adaptiveSyncEnabled: root.hasMonitor && monitor.hasSettings
            }

            Label {
                visible: root.hasMonitor && monitor.dirty && root.enabled
                text: qsTr("Unapplied edits are kept when switching backends.")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            GridLayout {
                columns: 2
                columnSpacing: 10
                Layout.fillWidth: true

                Label { text: qsTr("Name:") }
                Label {
                    id: nameValue
                    text: root.hasMonitor && monitor.name || ""
                    Layout.fillWidth: true
                }

                Label { text: qsTr("Description:") }
                Label {
                    id: descriptionValue
                    text: root.hasMonitor && monitor.description || ""
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }
            }

            GridLayout {
                visible: root.hasMonitor && monitor.hasSettings
                columns: 2
                columnSpacing: 10
                Layout.fillWidth: true
                enabled: root.hasMonitor && monitor.hasSettings

                Label { text: qsTr("Resolution:") }
                RowLayout {
                    Layout.fillWidth: true
                    ComboBox {
                        id: resolutionCombo
                        objectName: "resolutionCombo"
                        Layout.fillWidth: true
                        model: root.hasMonitor && monitor.resolutions || []
                        currentIndex: {
                            if (!root.hasMonitor || !monitor.hasSettings || count === 0) {
                                return -1
                            }
                            return monitor.activeResolutionIndex
                        }
                        onActivated: function(index) {
                            if (root.hasMonitor) {
                                monitor.activeResolutionIndex = index
                            }
                        }
                    }
                    Button {
                        id: preferredModeButton
                        text: qsTr("Preferred")
                        ToolTip.text: qsTr("Set preferred/native mode")
                        onClicked: {
                            if (root.hasMonitor) {
                                monitor.activate_preferred_mode()
                            }
                        }
                    }
                }

                OutputSettingsForm {
                    Layout.columnSpan: 2
                    item: monitor
                    settingsEnabled: root.hasMonitor && monitor.hasSettings
                }

                Label { text: qsTr("Position:") }
                RowLayout {
                    Layout.fillWidth: true
                    SpinBox {
                        id: posXSpinBox
                        objectName: "positionXSpinBox"
                        from: root.minimumPosition
                        to: root.maximumPosition
                        value: root.hasMonitor && monitor.positionX || 0
                        onValueModified: {
                            if (root.hasMonitor) {
                                monitor.positionX = value
                            }
                        }
                        editable: true
                    }
                    Label { text: "x" }
                    SpinBox {
                        id: posYSpinBox
                        objectName: "positionYSpinBox"
                        from: root.minimumPosition
                        to: root.maximumPosition
                        value: root.hasMonitor && monitor.positionY || 0
                        onValueModified: {
                            if (root.hasMonitor) {
                                monitor.positionY = value
                            }
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
