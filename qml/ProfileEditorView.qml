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
        width: 420
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
        width: 420
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
            if (pendingAction === 1) {
                profileEditor.reload_config_from_disk()
            } else if (pendingAction === 2) {
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

        ScrollView {
            id: profileFormScroll
            objectName: "profileFormScroll"
            Layout.minimumWidth: 360
            Layout.preferredWidth: 420
            Layout.minimumHeight: 0
            contentWidth: availableWidth
            clip: true

            ColumnLayout {
                width: profileFormScroll.availableWidth
                height: Math.max(implicitHeight, profileFormScroll.availableHeight)

                Label {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 0
                    visible: profileEditor.loadError.length > 0
                    text: profileEditor.loadError
                    textFormat: Text.PlainText
                    wrapMode: Text.WordWrap
                    color: palette.highlight
                }

                Button {
                    visible: !profileEditor.configurationLoaded
                    text: qsTr("Retry loading configuration")
                    onClicked: {
                        if (profileEditor.isDirty) {
                            discardDialog.pendingAction = 1
                            discardDialog.open()
                        } else {
                            profileEditor.reload_config_from_disk()
                        }
                    }
                }

                Label {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 0
                    text: profileEditor.configurationNotice
                    wrapMode: Text.WordWrap
                    color: palette.placeholderText
                }

                RowLayout {
                    Layout.fillWidth: true

                    Label { text: qsTr("Profile:") }

                    ComboBox {
                        id: profileCombo
                        objectName: "profileCombo"
                        Layout.fillWidth: true
                        model: profileEditor.profileIds

                        function syncIndex() {
                            var idx = model.indexOf(profileEditor.selectedProfileId)
                            if (idx >= 0) {
                                currentIndex = idx
                            } else {
                                currentIndex = 0
                            }
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
                        enabled: profileEditor.selectedProfileId.length > 0 && profileEditor.configurationLoaded
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
                    visible: profileEditor.supportsOnNoMatchExec
                    Layout.fillWidth: true
                    text: qsTr("The daemon runs these shell commands as its user when no profile matches. Saving reloads the daemon and may trigger them.")
                    wrapMode: Text.WordWrap
                }

                Label {
                    text: qsTr("Exec commands")
                    font.bold: true
                }

                Label {
                    Layout.fillWidth: true
                    text: qsTr("The daemon runs these commands through a shell as its user when activating a profile. Saving reloads the daemon and may trigger them. Loading or editing here does not run them.")
                    wrapMode: Text.WordWrap
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
                        objectName: "profileSaveButton"
                        text: qsTr("Save")
                        highlighted: true
                        enabled: profileEditor.isDirty && profileEditor.configurationLoaded
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
                    visible: profileEditor.saveStatusText.length > 0
                    text: profileEditor.saveStatusText
                    textFormat: Text.PlainText
                    wrapMode: Text.WordWrap
                }

                Label {
                    Layout.fillWidth: true
                    text: profileEditor.daemonStatusText
                    textFormat: Text.PlainText
                    wrapMode: Text.WordWrap
                    color: palette.placeholderText
                }
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
