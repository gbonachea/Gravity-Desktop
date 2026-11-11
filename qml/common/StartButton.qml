import QtQuick
import "Theme.js" as Theme

Item {
    id: root

    signal clicked()

    implicitWidth: 122
    implicitHeight: 46

    Rectangle {
        id: glow
        anchors.fill: parent
        anchors.margins: -4
        radius: height / 2
        color: "#334fa9ff"
        opacity: mouse.containsMouse ? 1.0 : 0.0
        visible: opacity > 0.01

        Behavior on opacity { NumberAnimation { duration: Theme.normal } }
    }

    Rectangle {
        anchors.fill: parent
        radius: height / 2
        border.width: 1
        border.color: mouse.containsMouse ? "#a9d7ff" : "#668fc8ff"
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#73b9ff" }
            GradientStop { position: 0.46; color: "#2a7cf0" }
            GradientStop { position: 1.0; color: "#0d449e" }
        }

        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            anchors.bottomMargin: parent.height * 0.45
            radius: parent.radius - 1
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#55ffffff" }
                GradientStop { position: 1.0; color: "#00ffffff" }
            }
        }
    }

    Row {
        anchors.centerIn: parent
        spacing: 9

        XpFlag {
            width: 27
            height: 27
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            text: qsTr("Inicio")
            anchors.verticalCenter: parent.verticalCenter
            color: "#ffffff"
            font.family: Theme.fontFamily
            font.pixelSize: 15
            font.weight: Font.DemiBold
            style: Text.Raised
            styleColor: "#66123b70"
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    scale: mouse.pressed ? 0.965 : 1.0
    Behavior on scale { NumberAnimation { duration: Theme.fast } }
}
