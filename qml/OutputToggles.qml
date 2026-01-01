import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GridLayout {
    id: root

    property var item
    property bool showAdaptiveSync: true
    property bool adaptiveSyncEnabled: true

    readonly property bool hasItem: item !== null

    columns: 2
    columnSpacing: 10
    Layout.fillWidth: true

    Label { text: qsTr("Enabled:") }
    Switch {
        checked: hasItem && item.enabled
        onCheckedChanged: {
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
        visible: showAdaptiveSync
        enabled: adaptiveSyncEnabled
        checked: hasItem && item.adaptiveSync
        onCheckedChanged: {
            if (hasItem && adaptiveSyncEnabled && item.adaptiveSync !== checked) {
                item.adaptiveSync = checked
            }
        }
    }
}
