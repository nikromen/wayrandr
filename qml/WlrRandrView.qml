import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import com.nikromen.wayrandr 1.0

Item {
    id: root

    required property var controller
    property string selectedMonitorName: ""

    function selectMonitor(index) {
        const models = controller.monitors
        if (index >= 0 && index < models.length && models[index].connected) {
            selectedMonitorName = models[index].name
            monitorTabBar.currentIndex = index
            return
        }
        selectedMonitorName = ""
        monitorTabBar.currentIndex = -1
    }

    function restoreSelection() {
        const models = controller.monitors
        let first = -1
        for (let i = 0; i < models.length; ++i) {
            if (models[i].connected) {
                if (first < 0) {
                    first = i
                }
                if (models[i].name === selectedMonitorName) {
                    selectMonitor(i)
                    return
                }
            }
        }
        selectMonitor(first)
    }

    Component.onCompleted: restoreSelection()

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        ColumnLayout {
            id: leftPanel
            Layout.minimumWidth: 280
            Layout.minimumHeight: 0

            TabBar {
                id: monitorTabBar
                objectName: "monitorTabBar"
                Layout.fillWidth: true

                Repeater {
                    model: controller.monitors
                    TabButton {
                        objectName: "monitorTab_" + modelData.name
                        enabled: modelData.connected
                        text: {
                            if (!modelData.connected) {
                                return modelData.name + qsTr(" (disconnected)")
                            }
                            return modelData.name
                        }
                        onClicked: root.selectedMonitorName = modelData.name
                        onEnabledChanged: Qt.callLater(root.restoreSelection)
                    }
                }
            }

            Connections {
                target: controller
                function onSwitch_to_monitor_tab(index) {
                    root.selectMonitor(index)
                }
                // Repeaters may reset the index while replacing their delegates.
                function onMonitors_changed() { Qt.callLater(root.restoreSelection) }
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
                        enabled: controller.directEditingAllowed && modelData.connected
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
            enabled: root.controller.directEditingAllowed
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
