import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import io.github.nikromen.wayrandr 1.0

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
                    checked: monitor.enabled
                    onCheckedChanged: monitor.enabled = checked
                }

                Switch {
                    id: flippedSwitch
                    visible: monitor.hasSettings
                    text: qsTr("Flipped")
                    checked: monitor.hasSettings ? monitor.flipped : false
                    onCheckedChanged: if (monitor.hasSettings) monitor.flipped = checked
                }

                Switch {
                    id: adaptiveSyncSwitch
                    visible: monitor.hasSettings
                    text: qsTr("Adaptive Sync")
                    checked: monitor.hasSettings ? monitor.adaptiveSync : false
                    onCheckedChanged: if (monitor.hasSettings) monitor.adaptiveSync = checked
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
            }


            GridLayout {
                visible: monitor.hasSettings
                columns: 2
                columnSpacing: 10
                Layout.fillWidth: true

                Label { text: qsTr("Resolution:") }
                RowLayout {
                    Layout.fillWidth: true
                    ComboBox {
                        id: resolutionCombo
                        Layout.fillWidth: true
                        model: monitor.resolutions
                        currentIndex: monitor.hasSettings ? monitor.activeResolutionIndex : 0
                        onCurrentIndexChanged: if (monitor.hasSettings) monitor.activeResolutionIndex = currentIndex
                        enabled: monitor.hasSettings
                    }
                    Button {
                        id: preferredModeButton
                        text: qsTr("Preferred")
                        ToolTip.text: qsTr("Set preferred/native mode")
                        enabled: monitor.hasSettings
                    }
                }

                Label { text: qsTr("Scale:") }
                SpinBox {
                    id: scaleSpinBox
                    Layout.fillWidth: true
                    from: 1
                    to: 150
                    stepSize: 1
                    value: monitor.hasSettings ? Math.round(monitor.scale * 10) : 10
                    onValueChanged: if (monitor.hasSettings) monitor.scale = value / 10
                    enabled: monitor.hasSettings

                    property int decimals: 1
                    
                    validator: DoubleValidator {
                        bottom: 1
                    }

                    textFromValue: function(value, locale) {
                        return Number(value / 10).toLocaleString(locale, 'f', scaleSpinBox.decimals)
                    }

                    valueFromText: function(value, locale) {
                        return Number.fromLocaleString(locale, text) * 10
                    }
                }

                Label { text: qsTr("Position:") }
                RowLayout {
                    Layout.fillWidth: true
                    SpinBox {
                        id: posXSpinBox
                        from: 0
                        to: 1000000
                        value: monitor.hasSettings ? monitor.positionX : 0
                        onValueChanged: if (monitor.hasSettings) monitor.positionX = value
                        enabled: monitor.hasSettings
                    }
                    Label { text: "x" }
                    SpinBox {
                        id: posYSpinBox
                        from: 0
                        to: 1000000
                        value: monitor.hasSettings ? monitor.positionY : 0
                        onValueChanged: if (monitor.hasSettings) monitor.positionY = value
                        enabled: monitor.hasSettings
                    }
                }

                Label { text: qsTr("Transform:") }
                ComboBox {
                    id: transformCombo
                    Layout.fillWidth: true
                    model: monitor.transformList
                    currentIndex: {
                        if (monitor.hasSettings && monitor.transform) {
                            var idx = model.indexOf(monitor.transform);
                            return idx !== -1 ? idx : 0;
                        }
                        return 0;
                    }
                    onCurrentIndexChanged: {
                        if (monitor.hasSettings && currentIndex !== -1) {
                            monitor.transform = model[currentIndex]
                        }
                    }
                    enabled: monitor.hasSettings
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
