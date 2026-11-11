import QtQuick
import "Theme.js" as Theme

Item {
    id: root

    property string text: ""
    property string glyph: ""
    property color foreground: Theme.textPrimary
    property color baseColor: "#14ffffff"
    property color hoverColor: "#2effffff"
    property color borderColor: "#33ffffff"
    property alias font: label.font
    property bool hovered: mouse.containsMouse

    signal clicked()

    implicitWidth: Math.max(96, contentRow.implicitWidth + 28)
    implicitHeight: 38

    Rectangle {
        anchors.fill: parent
        radius: 10
        color: mouse.pressed ? Theme.pressed : (mouse.containsMouse ? root.hoverColor : root.baseColor)
        border.width: 1
        border.color: root.borderColor

        Behavior on color { ColorAnimation { duration: Theme.fast } }
    }

    Row {
        id: contentRow
        anchors.centerIn: parent
        spacing: 8

        Text {
            visible: root.glyph.length > 0
            text: root.glyph
            color: root.foreground
            font.pixelSize: 15
            font.family: Theme.fontFamily
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            id: label
            text: root.text
            color: root.foreground
            font.pixelSize: 12
            font.family: Theme.fontFamily
            font.weight: Font.DemiBold
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    scale: mouse.pressed ? 0.97 : 1.0
    Behavior on scale { NumberAnimation { duration: Theme.fast } }
}
