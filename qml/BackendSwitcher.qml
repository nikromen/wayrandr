import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    id: root

    required property var controller

    readonly property int backendModeWlrRandr: 0
    readonly property int backendModeAutoWlrRandr: 2

    property int requestedBackendMode: -1

    spacing: 8

    ButtonGroup {
        id: backendButtonGroup
        buttons: [wlrRadio, autoRadio]
    }

    RadioButton {
        id: wlrRadio
        text: "wlr-randr"
        checked: controller.backendMode === root.backendModeWlrRandr
        onClicked: root.requestBackendSwitch(root.backendModeWlrRandr)
    }

    RadioButton {
        id: autoRadio
        text: "auto-wlr-randr"
        checked: controller.backendMode === root.backendModeAutoWlrRandr
        onClicked: root.requestBackendSwitch(root.backendModeAutoWlrRandr)
    }

    Dialog {
        id: backendDiscardDialog
        modal: true
        standardButtons: Dialog.Yes | Dialog.No
        title: qsTr("Discard changes?")

        contentItem: Label {
            text: qsTr("You have unsaved profile changes. Discard them and switch backend?")
            wrapMode: Text.WordWrap
            padding: 12
        }

        onAccepted: {
            controller.profileEditor.discard_changes()
            controller.set_backend_mode(root.requestedBackendMode)
            root.requestedBackendMode = -1
        }

        onRejected: {
            root.requestedBackendMode = -1
        }
    }

    function requestBackendSwitch(mode) {
        if (mode === controller.backendMode) {
            return
        }

        if (mode === root.backendModeAutoWlrRandr && controller.confirmationPending) {
            return
        }

        if (controller.backendMode === root.backendModeAutoWlrRandr &&
            controller.profileEditor.isDirty) {
            root.requestedBackendMode = mode
            backendDiscardDialog.open()
            return
        }

        if (!controller.can_switch_backend(mode)) {
            return
        }

        controller.set_backend_mode(mode)
    }
}
