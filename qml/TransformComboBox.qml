import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ComboBox {
    id: root

    property var item
    readonly property bool hasItem: item !== null

    model: {
        if (hasItem) {
            return item.transformList
        }
        return []
    }
    Layout.fillWidth: true

    Component.onCompleted: syncFromItem()

    onActivated: {
        if (hasItem) {
            item.transform = model[currentIndex]
        }
    }

    function syncFromItem() {
        if (!hasItem) {
            return
        }
        currentIndex = Math.max(0, model.indexOf(item.transform))
    }

    Connections {
        target: root.item
        enabled: root.hasItem
        function onTransform_changed() { root.syncFromItem() }
    }
}
