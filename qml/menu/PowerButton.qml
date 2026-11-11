import QtQuick
import "../common/Theme.js" as Theme

Item {
    id: root

    property string glyph: "⏻"
    property string label: qsTr("Apagar")
    property bool danger: false
    property bool hovered: mouse.containsMouse

    signal clicked()

    implicitWidth: content.implicitWidth + 24
    implicitHeight: 34

    Rectangle {
        anchors.fill: parent
        radius: 9
        color: mouse.containsMouse
            ? (root.danger ? "#44ff6b7a" : "#2effffff")
            : "#12ffffff"
        border.width: 1
        border.color: root.danger ? "#55ff9aa5" : "#2effffff"

        Behavior on color { ColorAnimation { duration: Theme.fast } }
    }

    Row {
        id: content
        anchors.centerIn: parent
        spacing: 6

        Text {
            text: root.glyph
            color: root.danger ? "#ffd3d8" : "#e8f3ff"
            font.pixelSize: 14
            font.family: Theme.fontFamily
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            text: root.label
            color: root.danger ? "#ffd3d8" : "#e8f3ff"
            font.pixelSize: 11
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
}
