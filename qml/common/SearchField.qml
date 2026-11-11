import QtQuick
import "Theme.js" as Theme

Item {
    id: root

    property alias text: input.text
    property string placeholder: qsTr("Buscar")
    property color foreground: Theme.textPrimary
    property color placeholderColor: Theme.textMuted
    property bool hovered: mouse.containsMouse

    signal accepted()
    signal escapePressed()

    implicitHeight: 36
    implicitWidth: 220

    function forceInputFocus() {
        input.forceActiveFocus()
    }

    Rectangle {
        anchors.fill: parent
        radius: 10
        color: input.activeFocus ? "#33ffffff" : (mouse.containsMouse ? "#22ffffff" : "#16ffffff")
        border.width: 1
        border.color: input.activeFocus ? "#88c9e9ff" : "#3a9fc7ff"

        Behavior on color { ColorAnimation { duration: Theme.fast } }
    }

    Text {
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.right: parent.right
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        text: "⌕"
        color: root.placeholderColor
        font.pixelSize: 18
        font.family: Theme.fontFamily
    }

    TextInput {
        id: input
        anchors.left: parent.left
        anchors.leftMargin: 34
        anchors.right: parent.right
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        color: root.foreground
        selectionColor: Theme.accentStrong
        selectedTextColor: "#ffffff"
        font.family: Theme.fontFamily
        font.pixelSize: 13
        clip: true
        onAccepted: root.accepted()
        Keys.onEscapePressed: root.escapePressed()
    }

    Text {
        anchors.left: input.left
        anchors.right: input.right
        anchors.verticalCenter: parent.verticalCenter
        text: root.placeholder
        color: root.placeholderColor
        font.family: Theme.fontFamily
        font.pixelSize: 13
        elide: Text.ElideRight
        visible: input.text.length === 0
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        cursorShape: Qt.IBeamCursor
        onClicked: input.forceActiveFocus()
    }
}
