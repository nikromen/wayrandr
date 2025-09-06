import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    visible: true
    
    // Dynamic sizing - window will adapt to content
    width: Screen.desktopAvailableWidth * 0.8
    height: Screen.desktopAvailableHeight * 0.8
    minimumWidth: 640
    minimumHeight: 480
    
    title: qsTr("WayRandR")
    
    // Use MainWindow.qml as the main content
    MainWindow {
        anchors.fill: parent
    }
    
    // Menu bar with common actions
    menuBar: MenuBar {
        Menu {
            title: qsTr("&File")
            Action { text: qsTr("&New Configuration") }
            Action { text: qsTr("&Open Configuration...") }
            Action { text: qsTr("&Save Configuration") }
            MenuSeparator { }
            Action { 
                text: qsTr("&Quit")
                onTriggered: window.close()
            }
        }
        Menu {
            title: qsTr("&View")
            Action { text: qsTr("&Refresh Monitors") }
        }
        Menu {
            title: qsTr("&Help")
            Action { text: qsTr("&About") }
        }
    }
    
    // Status bar for application messages
    footer: ToolBar {
        RowLayout {
            anchors.fill: parent
            Label {
                text: qsTr("Ready")
                Layout.fillWidth: true
                leftPadding: 10
            }
        }
    }
}
