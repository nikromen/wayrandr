import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    id: root

    required property var controller

    readonly property int backendModeWlrRandr: 0
    readonly property int backendModeKanshi: 1
    readonly property int backendModeAutoWlrRandr: 2

    property int requestedBackendMode: -1

    readonly property bool isProfileBackendMode:
        controller.backendMode === root.backendModeKanshi ||
        controller.backendMode === root.backendModeAutoWlrRandr

    enabled: !controller.operationBusy
    spacing: 8

    ButtonGroup {
        id: backendButtonGroup
    }

    Component.onCompleted: {
        const buttons = [wlrRadio]
        if (controller.kanshiAvailable) {
            buttons.push(kanshiRadio)
        }
        if (controller.autoWlrRandrAvailable) {
            buttons.push(autoRadio)
        }
        backendButtonGroup.buttons = buttons
    }

    RadioButton {
        id: wlrRadio
        text: "wlr-randr"
        checked: controller.backendMode === root.backendModeWlrRandr
        onClicked: root.requestBackendSwitch(root.backendModeWlrRandr)
    }

    RadioButton {
        id: kanshiRadio
        visible: controller.kanshiAvailable
        text: "kanshi"
        checked: controller.backendMode === root.backendModeKanshi
        onClicked: root.requestBackendSwitch(root.backendModeKanshi)
    }

    RadioButton {
        id: autoRadio
        visible: controller.autoWlrRandrAvailable
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

        if (mode === root.backendModeKanshi && !controller.kanshiAvailable) {
            return
        }

        if (mode === root.backendModeAutoWlrRandr && !controller.autoWlrRandrAvailable) {
            return
        }

        if (mode === root.backendModeAutoWlrRandr && controller.confirmationPending) {
            return
        }

        if (root.isProfileBackendMode && controller.profileEditor.isDirty) {
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
