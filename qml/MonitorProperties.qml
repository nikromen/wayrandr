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
                    visible: enabledSwitch.checked
                    text: qsTr("Flipped")
                    onCheckedChanged: monitor.flipped = checked
                }

                Switch {
                    id: adaptiveSyncSwitch
                    visible: enabledSwitch.checked
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
            }


            GridLayout {
                visible: enabledSwitch.checked
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
                    from: 1
                    to: 150
                    stepSize: 1
                    value: Math.round(monitor.scale * 10)
                    onValueChanged: monitor.scale = value / 10

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
                        value: monitor.positionX
                        onValueChanged: monitor.positionX = value
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

                Label { text: qsTr("Transform:") }
                ComboBox {
                    id: transformCombo
                    Layout.fillWidth: true
                    model: monitor.transformList
                    // TODO: fix this via offering index of transform - it is enum so it is constant
                    currentIndex: model.indexOf(monitor.transform)
                    onCurrentIndexChanged: {
                        if (currentIndex !== -1) {
                            monitor.transform = model[currentIndex]
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
