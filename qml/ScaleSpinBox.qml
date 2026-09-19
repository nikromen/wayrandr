import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

SpinBox {
    id: root
    objectName: "scaleSpinBox"

    property var item
    readonly property bool hasItem: !!item
    enabled: hasItem

    from: 1
    to: 150
    editable: true
    textFromValue: function(value, locale) {
        return Number(value / 10).toLocaleString(locale, 'f', 1)
    }
    valueFromText: function(text, locale) {
        return Number.fromLocaleString(locale, text) * 10
    }

    onValueModified: {
        if (hasItem) {
            item.scale = value / 10
        }
    }

    onItemChanged: syncFromItem()

    Component.onCompleted: syncFromItem()

    function syncFromItem() {
        if (!item) {
            value = from
            return
        }
        const scaledValue = Math.round(item.scale * 10)
        if (isFinite(scaledValue)) {
            value = Math.max(from, Math.min(to, scaledValue))
        } else {
            value = from
        }
    }

    Connections {
        target: root.item || null
        enabled: root.hasItem
        function onScale_changed() { root.syncFromItem() }
    }
}
