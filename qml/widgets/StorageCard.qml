import QtQuick
import "../common"
import "../common/Theme.js" as Theme

GlassCard {
    id: root

    radius: Theme.radius
    tintTop: Theme.lightTop
    tintBottom: Theme.lightBottom
    borderColor: Theme.lightBorder
    shadowColor: Theme.lightShadow
    shadowOffset: 8

    Item {
        id: diskIcon
        x: 18
        y: 18
        width: 38
        height: 38

        Rectangle {
            anchors.fill: parent
            radius: 10
            color: "#2a2a7cf0"
            border.width: 1
            border.color: "#552a7cf0"
        }

        Image {
            anchors.centerIn: parent
            width: 24
            height: 24
            source: "image://icon/drive-harddisk"
            sourceSize.width: 48
            sourceSize.height: 48
            fillMode: Image.PreserveAspectFit
            smooth: true
        }
    }

    Column {
        anchors.left: diskIcon.right
        anchors.leftMargin: 10
        anchors.right: parent.right
        anchors.rightMargin: 18
        y: 20
        spacing: 2

        Text {
            width: parent.width
            text: storage.name
            color: Theme.textOnLight
            font.family: Theme.fontFamily
            font.pixelSize: 13
            font.weight: Font.DemiBold
            elide: Text.ElideRight
        }

        Text {
            width: parent.width
            text: qsTr("Almacenamiento · %1").arg(storage.mountPath)
            color: Theme.textOnLightSecondary
            font.family: Theme.fontFamily
            font.pixelSize: 10
            elide: Text.ElideRight
        }
    }

    Text {
        anchors.right: parent.right
        anchors.rightMargin: 18
        y: 24
        text: Math.round(storage.percentUsed) + "%"
        color: Theme.accentStrong
        font.family: Theme.fontFamily
        font.pixelSize: 13
        font.weight: Font.DemiBold
    }

    Rectangle {
        id: track
        x: 18
        y: 78
        width: parent.width - 36
        height: 12
        radius: 6
        color: "#332a5d9c"
        border.width: 1
        border.color: "#22ffffff"

        Rectangle {
            id: fill
            x: 1
            y: 1
            height: parent.height - 2
            width: Math.max(6, (parent.width - 2) * Theme.clamp(storage.percentUsed, 0, 100) / 100)
            radius: height / 2

            gradient: Gradient {
                GradientStop { position: 0.0; color: "#8dc9ff" }
                GradientStop { position: 0.5; color: "#3d8bfd" }
                GradientStop { position: 1.0; color: "#1f5fc9" }
            }

            Behavior on width {
                NumberAnimation { duration: 500; easing.type: Easing.OutCubic }
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 1
                height: parent.height * 0.36
                radius: height / 2
                color: "#40ffffff"
            }
        }
    }

    Text {
        anchors.left: parent.left
        anchors.leftMargin: 18
        y: 106
        text: qsTr("%1 libres de %2").arg(storage.freeText).arg(storage.totalText)
        color: Theme.textOnLight
        font.family: Theme.fontFamily
        font.pixelSize: 11
    }

    Text {
        anchors.right: parent.right
        anchors.rightMargin: 18
        y: 106
        text: qsTr("%1 usados").arg(storage.usedText)
        color: Theme.textOnLightSecondary
        font.family: Theme.fontFamily
        font.pixelSize: 10
    }
}
