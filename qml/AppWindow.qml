import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    visible: true

    width: Screen.desktopAvailableWidth * 0.8
    height: Screen.desktopAvailableHeight * 0.8
    minimumWidth: 640
    minimumHeight: 480

    title: qsTr("wayrandr")

    onClosing: function(close) {
        close.accepted = mainWindow.prepare_close()
        if (close.accepted && mainWindow.profileEditor.isDirty) {
            close.accepted = false
            closeDiscardDialog.open()
        }
    }

    Dialog {
        id: closeDiscardDialog
        objectName: "closeDiscardDialog"
        anchors.centerIn: parent
        width: 420
        modal: true
        title: qsTr("Discard changes?")
        standardButtons: Dialog.Yes | Dialog.No
        contentItem: Label {
            text: qsTr("You have unsaved profile changes. Discard them and close?")
            wrapMode: Text.WordWrap
            padding: 12
        }
        onAccepted: {
            mainWindow.profileEditor.discard_changes()
            window.close()
        }
    }

    Connections {
        target: mainWindow
        function onClose_ready() { window.close() }
    }

    BackendStack {
        anchors.fill: parent
        controller: mainWindow
    }
}
