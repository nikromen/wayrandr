import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ScrollView {
    property var output
    property var profileEditor

    readonly property bool hasOutput: output !== null

    implicitHeight: mainLayout.implicitHeight
    visible: hasOutput

    Frame {
        width: parent.width

        ColumnLayout {
            id: mainLayout
            width: parent.width

            GridLayout {
                columns: 2
                columnSpacing: 10
                Layout.fillWidth: true

                Label { text: qsTr("Output pattern:") }
                TextField {
                    Layout.fillWidth: true
                    text: hasOutput ? output.outputPattern : ""
                    placeholderText: qsTr("e.g. DP-2 or Dell Inc. *")
                    onEditingFinished: {
                        if (hasOutput) {
                            output.outputPattern = text
                        }
                    }
                }

                Label {
                    Layout.columnSpan: 2
                    visible: profileEditor.supportsOutputMatchPreview
                    text: hasOutput ? output.matchPreview : ""
                    wrapMode: Text.WordWrap
                    color: palette.placeholderText
                }
            }

            OutputToggles {
                item: output
                showAdaptiveSync: profileEditor.supportsAdaptiveSync
                adaptiveSyncEnabled: hasOutput
            }

            GridLayout {
                visible: profileEditor.supportsPreferredMode
                columns: 2
                columnSpacing: 10
                Layout.fillWidth: true

                Label { text: qsTr("Preferred:") }
                Switch {
                    checked: hasOutput && output.preferred
                    onCheckedChanged: {
                        if (hasOutput && output.preferred !== checked) {
                            output.preferred = checked
                        }
                    }
                }
            }

            GridLayout {
                columns: 2
                columnSpacing: 10
                Layout.fillWidth: true

                Label { text: qsTr("Mode:") }
                TextField {
                    Layout.fillWidth: true
                    text: hasOutput ? output.mode : ""
                    placeholderText: qsTr("1920x1080@60Hz")
                    onTextChanged: {
                        if (hasOutput && text !== output.mode) {
                            output.mode = text
                        }
                    }
                }

                OutputSettingsForm {
                    Layout.columnSpan: 2
                    item: output
                    settingsEnabled: hasOutput
                }

                Label { text: qsTr("Position:") }
                RowLayout {
                    Layout.fillWidth: true
                    SpinBox {
                        from: 0
                        to: 1000000
                        editable: true
                        value: hasOutput ? output.positionX : 0
                        onValueModified: {
                            if (hasOutput && value !== output.positionX) {
                                output.positionX = value
                            }
                        }
                    }
                    Label { text: "x" }
                    SpinBox {
                        from: 0
                        to: 1000000
                        editable: true
                        value: hasOutput ? output.positionY : 0
                        onValueModified: {
                            if (hasOutput && value !== output.positionY) {
                                output.positionY = value
                            }
                        }
                    }
                }
            }
        }
    }
}
