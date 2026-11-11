import QtQuick
import "../common"
import "../common/Theme.js" as Theme

Item {
    id: root

    property string name: ""
    property string iconName: "application-x-executable"
    property string comment: ""
    property string desktopFile: ""

    signal activated()
    signal contextMenuRequested(string file, string name, real sceneX, real sceneY)

    width: ListView.view ? ListView.view.width : 0
    height: 47

    Rectangle {
        anchors.fill: parent
        anchors.leftMargin: 2
        anchors.rightMargin: 2
        radius: 10
        color: mouse.containsMouse ? Theme.hover : "transparent"
        border.width: mouse.containsMouse ? 1 : 0
        border.color: "#2e9fc7ff"

        Behavior on color { ColorAnimation { duration: Theme.fast } }
    }

    StellarIcon {
        id: icon
        x: 8
        anchors.verticalCenter: parent.verticalCenter
        iconName: root.iconName
        size: 30
    }

    Column {
        anchors.left: icon.right
        anchors.leftMargin: 10
        anchors.right: parent.right
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        spacing: 1

        Text {
            width: parent.width
            text: root.name
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: 13
            font.weight: Font.Medium
            elide: Text.ElideRight
        }

        Text {
            width: parent.width
            visible: root.comment.length > 0
            text: root.comment
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: 10
            elide: Text.ElideRight
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        cursorShape: Qt.PointingHandCursor
        onClicked: function(event) {
            if (event.button === Qt.RightButton) {
                const point = mouse.mapToItem(null, mouse.x, mouse.y)
                root.contextMenuRequested(root.desktopFile, root.name, point.x, point.y)
            } else {
                root.activated()
            }
        }
    }
}
