import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    required property var controller

    StackLayout {
        anchors.fill: parent
        currentIndex: controller.backendMode === 2 ? 1 : 0

        WlrRandrView {
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
