import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import com.nikromen.wayrandr 1.0

Item {
    id: root

    required property var controller

    readonly property var profileEditor: controller.profileEditor

    enabled: !profileEditor.operationBusy

    Component.onCompleted: profileEditor.refresh_connected_outputs()

    function promptProfileName(title, callback) {
        profileNameDialog.title = title
        profileNameDialog.callback = callback
        profileNameField.text = ""
        profileNameDialog.open()
    }

    Dialog {
        id: profileNameDialog
        property var callback: null
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        title: qsTr("Profile name")

        contentItem: TextField {
            id: profileNameField
            placeholderText: qsTr("home_office")
            width: 240
        }

        onAccepted: {
            if (profileNameDialog.callback && profileNameField.text.length > 0) {
                profileNameDialog.callback(profileNameField.text)
            }
        }
    }

    Dialog {
        id: discardDialog
        property int pendingAction: 0
        modal: true
        standardButtons: Dialog.Yes | Dialog.No
        title: qsTr("Discard changes?")

        contentItem: Label {
            text: qsTr("You have unsaved profile changes. Discard them?")
            wrapMode: Text.WordWrap
            padding: 12
        }

        onAccepted: {
            profileEditor.discard_changes()
            if (pendingAction === 2) {
                profileEditor.select_profile(pendingProfileId, true)
            }
            pendingAction = 0
            pendingProfileId = ""
        }

        onRejected: {
            pendingAction = 0
            pendingProfileId = ""
            profileCombo.syncIndex()
        }
    }

    property string pendingProfileId: ""

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        ColumnLayout {
            Layout.minimumWidth: 360
            Layout.preferredWidth: 420
            Layout.minimumHeight: 0

            RowLayout {
                Layout.fillWidth: true

                Label { text: qsTr("Profile:") }

                ComboBox {
                    id: profileCombo
                    Layout.fillWidth: true
                    model: profileEditor.profileIds

                    function syncIndex() {
                        var idx = model.indexOf(profileEditor.selectedProfileId)
                        currentIndex = idx >= 0 ? idx : 0
                    }

                    Component.onCompleted: syncIndex()

                    Connections {
                        target: profileEditor
                        function onSelected_profile_id_changed() {
                            profileCombo.syncIndex()
                        }
                    }

                    onActivated: {
                        if (!profileEditor.select_profile(model[currentIndex])) {
                            pendingAction = 2
                            pendingProfileId = model[currentIndex]
                            discardDialog.open()
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true

                Button {
                    text: qsTr("New from current")
                    onClicked: promptProfileName(qsTr("New profile name"), function(name) {
                        profileEditor.create_profile_from_live(name)
                    })
                }

                Button {
                    text: qsTr("Duplicate")
                    enabled: profileEditor.selectedProfileId.length > 0
                    onClicked: promptProfileName(qsTr("Duplicate profile as"), function(name) {
                        profileEditor.duplicate_profile(profileEditor.selectedProfileId, name)
                    })
                }

                Button {
                    text: qsTr("Delete")
                    enabled: profileEditor.selectedProfileId.length > 0
                    onClicked: profileEditor.delete_profile(profileEditor.selectedProfileId)
                }
            }

            TabBar {
                id: outputTabBar
                Layout.fillWidth: true

                Repeater {
                    model: profileEditor.outputs
                    TabButton {
                        text: modelData.outputPattern || qsTr("Output")
                    }
                }
            }

            Connections {
                target: profileEditor
                function onOutputs_changed() {
                    if (outputTabBar.currentIndex >= profileEditor.outputs.length) {
                        outputTabBar.currentIndex = Math.max(0, profileEditor.outputs.length - 1)
                    }
                }
            }

            ProfileOutputProperties {
                Layout.fillWidth: true
                Layout.fillHeight: true
                output: {
                    var idx = outputTabBar.currentIndex
                    var outputs = profileEditor.outputs
                    if (idx < 0 || idx >= outputs.length) {
                        return null
                    }
                    return outputs[idx]
                }
                profileEditor: root.profileEditor
            }

            RowLayout {
                Layout.fillWidth: true
                Button {
                    text: qsTr("Add output")
                    onClicked: profileEditor.add_output()
                }
                Button {
                    text: qsTr("Remove output")
                    enabled: outputTabBar.currentIndex >= 0
                    onClicked: profileEditor.remove_output(outputTabBar.currentIndex)
                }
            }

            Label {
                visible: profileEditor.supportsOnNoMatchExec
                text: qsTr("On no match exec")
                font.bold: true
            }

            CommandListEditor {
                visible: profileEditor.supportsOnNoMatchExec
                Layout.fillWidth: true
                commands: profileEditor.onNoMatchExecCommands
                onSetCommand: function(index, text) {
                    profileEditor.set_on_no_match_exec_command(index, text)
                }
                onRemoveCommand: function(index) {
                    profileEditor.remove_on_no_match_exec_command(index)
                }
                onAddCommand: function() {
                    profileEditor.add_on_no_match_exec_command("")
                }
            }

            Label {
                text: qsTr("Exec commands")
                font.bold: true
            }

            CommandListEditor {
                Layout.fillWidth: true
                commands: profileEditor.execCommands
                onSetCommand: function(index, text) {
                    profileEditor.set_exec_command(index, text)
                }
                onRemoveCommand: function(index) {
                    profileEditor.remove_exec_command(index)
                }
                onAddCommand: function() {
                    profileEditor.add_exec_command("")
                }
            }

            Label {
                Layout.fillWidth: true
                visible: profileEditor.matchWarning.length > 0
                text: profileEditor.matchWarning
                wrapMode: Text.WordWrap
                color: palette.highlight
            }

            RowLayout {
                Layout.fillWidth: true

                Button {
                    text: qsTr("Save")
                    highlighted: true
                    enabled: profileEditor.isDirty
                    onClicked: profileEditor.save_profile()
                }

                Button {
                    text: qsTr("Discard")
                    enabled: profileEditor.isDirty
                    onClicked: profileEditor.discard_changes()
                }

                CheckBox {
                    id: forceSwitchCheck
                    visible: profileEditor.supportsForceSwitch
                    text: qsTr("Force")
                    ToolTip.text: qsTr("Apply profile even if output patterns do not match")
                }

                Button {
                    text: qsTr("Switch profile")
                    enabled: profileEditor.selectedProfileId.length > 0
                    onClicked: profileEditor.switch_profile(forceSwitchCheck.checked)
                }
            }

            Label {
                Layout.fillWidth: true
                text: profileEditor.daemonStatusText
                wrapMode: Text.WordWrap
                color: palette.placeholderText
            }
        }

        DisplayCanvas {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 0
            Layout.minimumHeight: 0
            controller: root.controller
            canvasModel: profileEditor.outputs
            blockName: function(item, index) {
                return item.outputPattern
            }
            blockImageSource: function(item, index) {
                return ""
            }
            blockVisible: function(item, index) {
                return item.enabled
            }
            onBlockPressed: function(item, index) {
                outputTabBar.currentIndex = index
            }
        }
    }
}
