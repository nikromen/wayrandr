import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ComboBox {
    id: root
    objectName: "transformComboBox"

    property var item
    readonly property bool hasItem: !!item

    model: {
        if (hasItem) {
            return item.transformList
        }
        return []
    }
    Layout.fillWidth: true

    Component.onCompleted: syncFromItem()
    onItemChanged: syncFromItem()
    onModelChanged: syncFromItem()

    onActivated: {
        if (hasItem) {
            item.transform = model[currentIndex]
        }
    }

    function syncFromItem() {
        if (!item) {
            currentIndex = -1
            return
        }
        currentIndex = Math.max(0, model.indexOf(item.transform))
    }

    Connections {
        target: root.item || null
        enabled: root.hasItem
        function onTransform_changed() { root.syncFromItem() }
    }
}
