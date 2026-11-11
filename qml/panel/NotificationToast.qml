import QtQuick
import "../common"
import "../common/Theme.js" as Theme

/*!
 * Notificación emergente del panel. La vista la muestra/oculta el controlador
 * C++ y se cierra sola tras unos segundos.
 */
Item {
    id: root

    property var notification: ({})

    function show(item) {
        notification = item
        card.opacity = 0.0
        card.scale = 0.96
        intro.restart()
    }

    ParallelAnimation {
        id: intro
        NumberAnimation { target: card; property: "opacity"; from: 0.0; to: 1.0; duration: Theme.fast }
        NumberAnimation { target: card; property: "scale"; from: 0.96; to: 1.0; duration: Theme.normal; easing.type: Easing.OutCubic }
    }

    GlassCard {
        id: card
        anchors.fill: parent
        radius: 16
        tintTop: Theme.menuTop
        tintBottom: Theme.menuBottom
        borderColor: Theme.menuBorder
        shadowOffset: 10

        opacity: 1.0
        scale: 1.0

        MouseArea { anchors.fill: parent }

        StellarIcon {
            id: appIcon
            x: 16
            y: 18
            size: 42
            iconName: root.notification.appIcon !== undefined
                && root.notification.appIcon.length > 0
                ? root.notification.appIcon
                : "preferences-desktop-notification-bell"
        }

        Text {
            id: appName
            x: 70
            y: 16
            width: parent.width - 120
            text: root.notification.appName !== undefined && root.notification.appName.length > 0
                ? root.notification.appName
                : qsTr("Notificación")
            color: Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: 10
            font.weight: Font.DemiBold
            elide: Text.ElideRight
        }

        Text {
            id: summary
            x: 70
            y: 32
            width: parent.width - 120
            text: root.notification.summary !== undefined ? root.notification.summary : ""
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: 14
            font.weight: Font.DemiBold
            elide: Text.ElideRight
        }

        Text {
            id: bodyText
            x: 20
            y: 66
            width: parent.width - 40
            height: 62
            text: root.notification.body !== undefined ? root.notification.body : ""
            textFormat: Text.PlainText
            color: Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: 11
            wrapMode: Text.WordWrap
            maximumLineCount: 3
            elide: Text.ElideRight
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 20
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 12
            text: root.notification.timestamp !== undefined
                ? Qt.formatTime(new Date(root.notification.timestamp), "hh:mm")
                : ""
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: 10
        }

        Text {
            anchors.right: parent.right
            anchors.rightMargin: 18
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 10
            text: qsTr("Stellar")
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: 10
        }

        Item {
            id: closeButton
            width: 26
            height: 26
            anchors.right: parent.right
            anchors.rightMargin: 10
            anchors.top: parent.top
            anchors.topMargin: 8

            Rectangle {
                anchors.fill: parent
                radius: 13
                color: closeMouse.containsMouse ? "#33ffffff" : "transparent"
            }

            Text {
                anchors.centerIn: parent
                text: "✕"
                color: Theme.textSecondary
                font.pixelSize: 12
                font.family: Theme.fontFamily
            }

            MouseArea {
                id: closeMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: toast.hide()
            }
        }
    }
}
