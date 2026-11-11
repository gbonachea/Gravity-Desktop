import QtQuick
import "../common"
import "../common/Theme.js" as Theme

Item {
    id: root

    property bool opened: false
    property string lastCommand: ""

    signal commandRequested(string command)

    visible: opened
    z: 100
    opacity: opened ? 1 : 0

    Behavior on opacity { NumberAnimation { duration: Theme.fast } }

    function open() {
        opened = true
        lastCommand = ""
        input.text = ""
        input.forceActiveFocus()
    }

    function close() {
        opened = false
    }

    Rectangle {
        anchors.fill: parent
        color: "#66040f22"

        MouseArea {
            anchors.fill: parent
            onClicked: root.close()
        }
    }

    GlassCard {
        id: dialog
        width: 430
        height: 184
        anchors.centerIn: parent
        radius: Theme.radius
        tintTop: Theme.menuTop
        tintBottom: Theme.menuBottom
        borderColor: Theme.menuBorder

        Rectangle {
            anchors.fill: parent
            radius: dialog.radius
            color: "transparent"

            MouseArea { anchors.fill: parent }
        }

        Text {
            x: 20
            y: 16
            text: qsTr("Ejecutar")
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: 16
            font.weight: Font.DemiBold
        }

        Text {
            x: 20
            y: 42
            text: qsTr("Escribe un comando o la ruta de un programa.")
            color: Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: 11
        }

        Rectangle {
            x: 20
            y: 68
            width: parent.width - 40
            height: 38
            radius: 9
            color: "#24ffffff"
            border.width: 1
            border.color: input.activeFocus ? "#88c9e9ff" : "#3a9fc7ff"

            TextInput {
                id: input
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                color: Theme.textPrimary
                selectionColor: Theme.accentStrong
                selectedTextColor: "#ffffff"
                font.family: Theme.fontFamily
                font.pixelSize: 13
                verticalAlignment: TextInput.AlignVCenter
                clip: true
                onAccepted: root.run()
                Keys.onEscapePressed: root.close()
            }

            Text {
                anchors.left: input.left
                anchors.verticalCenter: input.verticalCenter
                text: qsTr("p. ej. firefox o xdg-open ~/Descargas")
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: 12
                visible: input.text.length === 0
            }
        }

        Row {
            anchors.right: parent.right
            anchors.rightMargin: 20
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 16
            spacing: 8

            GlassButton {
                text: qsTr("Cancelar")
                baseColor: "#12ffffff"
                hoverColor: "#2effffff"
                onClicked: root.close()
            }

            GlassButton {
                text: qsTr("Ejecutar")
                baseColor: "#4d2a7cf0"
                hoverColor: "#664fa9ff"
                borderColor: "#66a9d7ff"
                onClicked: root.run()
            }
        }
    }

    function run() {
        const command = input.text.trim()
        if (command.length === 0) {
            return
        }
        lastCommand = command
        commandRequested(command)
        close()
    }
}
