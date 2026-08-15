import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root

    property var commands
    property var onSetCommand
    property var onRemoveCommand
    property var onAddCommand

    Layout.fillWidth: true

    Repeater {
        model: root.commands
        RowLayout {
            Layout.fillWidth: true
            TextField {
                objectName: "commandField"
                Layout.fillWidth: true
                text: modelData
                onEditingFinished: {
                    if (root.onSetCommand) {
                        root.onSetCommand(index, text)
                    }
                }
            }
            ToolButton {
                objectName: "removeCommandButton"
                text: "×"
                Accessible.name: qsTr("Remove command")
                onClicked: {
                    if (root.onRemoveCommand) {
                        root.onRemoveCommand(index)
                    }
                }
            }
        }
    }

    Button {
        objectName: "addCommandButton"
        text: qsTr("Add command")
        onClicked: {
            if (root.onAddCommand) {
                root.onAddCommand()
            }
        }
    }
}
