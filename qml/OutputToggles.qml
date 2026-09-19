import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GridLayout {
    id: root

    property var item
    property bool showAdaptiveSync: true
    property bool adaptiveSyncEnabled: true

    readonly property bool hasItem: !!item
    enabled: hasItem

    columns: 2
    columnSpacing: 10
    Layout.fillWidth: true

    Label { text: qsTr("Enabled:") }
    Switch {
        objectName: "outputEnabledSwitch"
        checked: hasItem && item.enabled
        onToggled: {
            if (hasItem && item.enabled !== checked) {
                item.enabled = checked
            }
        }
    }

    Label {
        visible: showAdaptiveSync
        text: qsTr("Adaptive sync:")
    }
    Switch {
        objectName: "adaptiveSyncSwitch"
        visible: showAdaptiveSync
        enabled: adaptiveSyncEnabled
        checked: hasItem && item.adaptiveSync
        onToggled: {
            if (hasItem && adaptiveSyncEnabled && item.adaptiveSync !== checked) {
                item.adaptiveSync = checked
            }
        }
    }
}
