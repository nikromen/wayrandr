import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import com.nikromen.wayrandr 1.0

Item {
    id: root

    required property var controller

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        ColumnLayout {
            id: leftPanel
            Layout.minimumWidth: 280
            Layout.minimumHeight: 0

            TabBar {
                id: monitorTabBar
                Layout.fillWidth: true

                Repeater {
                    model: controller.monitors
                    TabButton {
                        text: modelData.name
                    }
                }
            }

            Connections {
                target: controller
                function onSwitch_to_monitor_tab(index) {
                    monitorTabBar.currentIndex = index
                }
            }

            StackLayout {
                id: monitorStackLayout
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: monitorTabBar.currentIndex

                Repeater {
                    model: controller.monitors
                    MonitorProperties {
                        monitor: modelData
                        width: parent.width
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                objectName: "applyError"
                visible: controller.applyError.length > 0
                text: controller.applyError
                textFormat: Text.PlainText
                wrapMode: Text.WordWrap
            }

            RowLayout {
                spacing: 10
                Layout.fillWidth: true

                Button {
                    objectName: "applyButton"
                    text: {
                        if (controller.operationBusy) {
                            return qsTr("Working…")
                        }
                        return qsTr("Apply")
                    }
                    enabled: !controller.operationBusy
                    highlighted: true
                    Layout.fillWidth: true
                    visible: !controller.confirmationPending
                    onClicked: controller.apply()
                }

                Button {
                    objectName: "confirmButton"
                    text: qsTr("Confirm")
                    highlighted: true
                    Layout.fillWidth: true
                    visible: controller.confirmationPending
                    enabled: controller.confirmationAllowed
                    onClicked: controller.confirm_apply()
                }

                Button {
                    objectName: "cancelButton"
                    text: {
                        if (controller.confirmationAllowed) {
                            return qsTr("Cancel")
                        }
                        return qsTr("Retry restore")
                    }
                    Layout.fillWidth: true
                    visible: controller.confirmationPending
                    onClicked: controller.cancel_apply()
                }

                Label {
                    visible: controller.confirmationPending && controller.confirmationSecondsLeft > 0
                    text: qsTr("Reverting in %1 s").arg(controller.confirmationSecondsLeft)
                }
            }
        }

        DisplayCanvas {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 0
            Layout.minimumHeight: 0
            controller: root.controller
            canvasModel: controller.monitors
            blockName: function(item, index) {
                return item.name
            }
            blockImageSource: function(item, index) {
                var block = controller.get_monitor_block(item.name)
                if (block) {
                    return block.screenImage
                }
                return ""
            }
            blockVisible: function(item, index) {
                return item.enabled
            }
            onBlockPressed: function(item, index) {
                var monitorIndex = controller.get_monitor_index(item)
                if (monitorIndex >= 0) {
                    controller.switch_to_monitor_tab(monitorIndex)
                }
            }
        }
    }
}
