import QtQuick
import "Theme.js" as Theme

Item {
    id: root

    property int level: 100
    property bool charging: false
    property bool present: true
    property color outlineColor: "#d9edff"

    implicitWidth: 30
    implicitHeight: 16

    Rectangle {
        id: body
        anchors.fill: parent
        anchors.rightMargin: 4
        radius: 4
        color: "transparent"
        border.width: 1.4
        border.color: root.outlineColor
    }

    Rectangle {
        x: 2.5
        anchors.verticalCenter: parent.verticalCenter
        width: Math.max(2, (body.width - 5) * Theme.clamp(root.level, 0, 100) / 100)
        height: body.height - 5
        radius: 2
        gradient: Gradient {
            GradientStop { position: 0.0; color: root.level <= 15 ? "#ff8b8b" : "#8fe8b0" }
            GradientStop { position: 1.0; color: root.level <= 15 ? "#e14e5e" : "#39b96a" }
        }

        Behavior on width { NumberAnimation { duration: Theme.normal } }
    }

    Rectangle {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        width: 3
        height: parent.height * 0.38
        radius: 1
        color: root.outlineColor
    }

    Text {
        visible: root.charging
        anchors.centerIn: body
        text: "ϟ"
        color: "#0b2b50"
        font.pixelSize: 12
        font.bold: true
    }
}
