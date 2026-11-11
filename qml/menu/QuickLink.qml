import QtQuick
import "../common"
import "../common/Theme.js" as Theme

Item {
    id: root

    property string label: ""
    property string glyph: "▣"
    property string iconName: ""
    property color glyphColor: Theme.accent
    property bool hovered: mouse.containsMouse

    signal clicked()

    height: 40

    Rectangle {
        anchors.fill: parent
        radius: 9
        color: mouse.containsMouse ? Theme.hover : "transparent"
        border.width: mouse.containsMouse ? 1 : 0
        border.color: "#2e9fc7ff"

        Behavior on color { ColorAnimation { duration: Theme.fast } }
    }

    Rectangle {
        id: glyphTile
        width: 28
        height: 28
        radius: 8
        anchors.left: parent.left
        anchors.leftMargin: 4
        anchors.verticalCenter: parent.verticalCenter
        color: Qt.rgba(root.glyphColor.r, root.glyphColor.g, root.glyphColor.b, 0.20)
        border.width: 1
        border.color: Qt.rgba(root.glyphColor.r, root.glyphColor.g, root.glyphColor.b, 0.45)

        Text {
            visible: root.iconName.length === 0
            anchors.centerIn: parent
            text: root.glyph
            color: "#ffffff"
            font.pixelSize: 15
            font.family: Theme.fontFamily
        }

        StellarIcon {
            visible: root.iconName.length > 0
            anchors.centerIn: parent
            size: 20
            showTile: false
            iconName: root.iconName
        }
    }

    Text {
        anchors.left: glyphTile.right
        anchors.leftMargin: 10
        anchors.right: arrow.left
        anchors.rightMargin: 4
        anchors.verticalCenter: parent.verticalCenter
        text: root.label
        color: Theme.textPrimary
        font.family: Theme.fontFamily
        font.pixelSize: 12
        elide: Text.ElideRight
    }

    Text {
        id: arrow
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        text: "›"
        color: mouse.containsMouse ? Theme.textPrimary : Theme.textMuted
        font.pixelSize: 17
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
