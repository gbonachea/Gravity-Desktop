import QtQuick
import "Theme.js" as Theme

Item {
    id: root

    property string title: ""
    property string iconName: "application-x-executable"
    property bool active: false
    property bool minimized: false
    property bool hovered: mouse.containsMouse

    signal activated()
    signal closeRequested()

    height: 46
    width: active ? 178 : 46

    Behavior on width { NumberAnimation { duration: Theme.normal; easing.type: Easing.OutCubic } }

    Rectangle {
        anchors.fill: parent
        radius: 12
        color: mouse.containsMouse ? Theme.hover : (root.active ? "#36ffffff" : "#0fffffff")
        border.width: 1
        border.color: root.active ? "#66b9e2ff" : "#1fffffff"

        Behavior on color { ColorAnimation { duration: Theme.fast } }
    }

    Row {
        anchors.left: parent.left
        anchors.leftMargin: 6
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8

        StellarIcon {
            iconName: root.iconName
            size: 32
            opacity: root.minimized ? 0.6 : 1.0
        }

        Text {
            visible: root.active
            width: Math.max(0, root.width - 62)
            text: root.title
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: 12
            elide: Text.ElideRight
            anchors.verticalCenter: parent.verticalCenter
            opacity: root.minimized ? 0.7 : 1.0
        }
    }

    Rectangle {
        visible: root.active
        width: 16
        height: 3
        radius: 1.5
        color: Theme.accent
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 2
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
        cursorShape: Qt.PointingHandCursor
        onClicked: function(event) {
            if (event.button === Qt.LeftButton) {
                root.activated()
            } else {
                root.closeRequested()
            }
        }
    }
}
