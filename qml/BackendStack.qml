import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import com.nikromen.wayrandr 1.0

Rectangle {
    id: root

    required property var controller

    function backendViewIndex(mode) {
        if (mode === EditorMode.WLR_RANDR) {
            return 0
        }
        if (mode === EditorMode.KANSHI) {
            return 1
        }
        if (mode === EditorMode.AUTO_WLR_RANDR) {
            return 2
        }
        return -1
    }

    StackLayout {
        anchors.fill: parent
        currentIndex: root.backendViewIndex(controller.backendMode)

        WlrRandrView {
            controller: root.controller
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 0
            Layout.minimumHeight: 0
        }

        KanshiView {
            controller: root.controller
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 0
            Layout.minimumHeight: 0
        }

        AutoWlrRandrView {
            controller: root.controller
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 0
            Layout.minimumHeight: 0
        }
    }
}
