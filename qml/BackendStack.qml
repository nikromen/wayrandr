import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    required property var controller

    function backendViewIndex(mode) {
        if (mode === 0) {
            return 0
        }
        if (mode === 1) {
            return 1
        }
        if (mode === 2) {
            return 2
        }
        return 0
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
