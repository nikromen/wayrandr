import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GridLayout {
    id: root

    property var item
    property bool settingsEnabled: true

    readonly property bool hasItem: !!item

    columns: 2
    columnSpacing: 10
    Layout.fillWidth: true
    enabled: settingsEnabled && hasItem

    Label { text: qsTr("Scale:") }
    ScaleSpinBox {
        item: root.item
        Layout.fillWidth: true
    }

    Label { text: qsTr("Transform:") }
    TransformComboBox {
        item: root.item
    }
}
