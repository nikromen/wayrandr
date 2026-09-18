import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import com.nikromen.wayrandr 1.0

RowLayout {
    id: root

    required property var controller

    property int requestedBackendMode: -1

    readonly property bool isProfileBackendMode:
        controller.backendMode === EditorMode.KANSHI ||
        controller.backendMode === EditorMode.AUTO_WLR_RANDR

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
        objectName: "wlrBackend"
        text: "wlr-randr"
        checked: controller.backendMode === EditorMode.WLR_RANDR
        onClicked: root.requestBackendSwitch(EditorMode.WLR_RANDR)
    }

    RadioButton {
        id: kanshiRadio
        objectName: "kanshiBackend"
        visible: controller.kanshiAvailable
        text: "kanshi"
        checked: controller.backendMode === EditorMode.KANSHI
        onClicked: root.requestBackendSwitch(EditorMode.KANSHI)
    }

    RadioButton {
        id: autoRadio
        objectName: "autoBackend"
        visible: controller.autoWlrRandrAvailable
        text: "auto-wlr-randr"
        checked: controller.backendMode === EditorMode.AUTO_WLR_RANDR
        onClicked: root.requestBackendSwitch(EditorMode.AUTO_WLR_RANDR)
    }

    Dialog {
        id: backendDiscardDialog
        objectName: "backendDiscardDialog"
        width: 420
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

        if (mode === EditorMode.KANSHI && !controller.kanshiAvailable) {
            return
        }

        if (mode === EditorMode.AUTO_WLR_RANDR && !controller.autoWlrRandrAvailable) {
            return
        }

        if (mode === EditorMode.AUTO_WLR_RANDR && controller.confirmationPending) {
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
