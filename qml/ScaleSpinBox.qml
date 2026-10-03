import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

SpinBox {
    id: root
    objectName: "scaleSpinBox"

    property var item
    readonly property bool hasItem: item !== null

    from: 1
    to: 150
    editable: true
    textFromValue: function(value, locale) {
        return Number(value / 10).toLocaleString(locale, 'f', 1)
    }
    valueFromText: function(text, locale) {
        return Number.fromLocaleString(locale, text) * 10
    }

    property bool updatingFromItem: false
    property bool ready: false

    onValueChanged: {
        if (!ready || !item || updatingFromItem) {
            return
        }
        item.scale = value / 10
    }

    onItemChanged: syncFromItem()

    Component.onCompleted: syncFromItem()

    function syncFromItem() {
        ready = false
        if (!item) {
            return
        }
        updatingFromItem = true
        value = Math.round(item.scale * 10)
        updatingFromItem = false
        ready = true
    }

    Connections {
        target: root.item
        enabled: root.hasItem
        function onScale_changed() { root.syncFromItem() }
    }
}
