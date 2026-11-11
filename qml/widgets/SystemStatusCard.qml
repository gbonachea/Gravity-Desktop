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

    // ------------------------------------------------------------------
    // Estado del sistema (sin notificaciones)
    // ------------------------------------------------------------------
    Item {
        id: healthContent
        anchors.fill: parent
        visible: notifications.count === 0

        Item {
            id: statusIcon
            x: 18
            y: 22
            width: 52
            height: 52

            Rectangle {
                anchors.fill: parent
                radius: width / 2
                color: health.healthy ? "#2e42c46e" : "#2eef7b8a"
                border.width: 1
                border.color: health.healthy ? "#6642c46e" : "#66ef7b8a"
            }

            Rectangle {
                anchors.centerIn: parent
                width: 34
                height: 34
                radius: width / 2
                gradient: Gradient {
                    GradientStop {
                        position: 0.0
                        color: health.healthy ? "#7ae6a4" : "#ff9aa8"
                    }
                    GradientStop {
                        position: 1.0
                        color: health.healthy ? "#2fa85b" : "#d94458"
                    }
                }

                Text {
                    anchors.centerIn: parent
                    text: health.healthy ? "✓" : "!"
                    color: "#ffffff"
                    font.pixelSize: 19
                    font.bold: true
                }
            }
        }

        Column {
            anchors.left: statusIcon.right
            anchors.leftMargin: 14
            anchors.right: parent.right
            anchors.rightMargin: 18
            y: 26
            spacing: 4

            Text {
                width: parent.width
                text: health.title
                color: Theme.textOnLight
                font.family: Theme.fontFamily
                font.pixelSize: 13
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }

            Text {
                width: parent.width
                text: health.detail
                color: Theme.textOnLightSecondary
                font.family: Theme.fontFamily
                font.pixelSize: 10
                wrapMode: Text.WordWrap
                maximumLineCount: 2
                elide: Text.ElideRight
            }
        }
    }

    // ------------------------------------------------------------------
    // Notificaciones: se muestra siempre la más reciente. Los puntos de la
    // izquierda permiten recuperar las anteriores; el botón ✕ las limpia.
    // ------------------------------------------------------------------
    Item {
        id: notificationContent
        anchors.fill: parent
        visible: notifications.count > 0

        ListView {
            id: notificationDots
            x: 8
            y: 12
            width: 18
            height: parent.height - 46
            clip: true
            spacing: 4
            model: notifications.count
            boundsBehavior: Flickable.StopAtBounds

            delegate: Item {
                width: notificationDots.width
                height: 10

                Rectangle {
                    width: index === notifications.currentIndex ? 9 : 6
                    height: width
                    radius: width / 2
                    anchors.centerIn: parent
                    color: index === notifications.currentIndex ? "#3f8fe0" : "#7a9ab8"
                    border.width: index === notifications.currentIndex ? 1 : 0
                    border.color: "#b9ddff"
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: notifications.select(index)
                }
            }
        }

        Item {
            id: clearButton
            x: 4
            y: parent.height - 30
            width: 26
            height: 24

            Rectangle {
                anchors.fill: parent
                radius: 7
                color: clearMouse.containsMouse ? "#33ffffff" : "transparent"
            }

            Text {
                anchors.centerIn: parent
                text: "✕"
                color: Theme.textOnLightSecondary
                font.family: Theme.fontFamily
                font.pixelSize: 11
            }

            MouseArea {
                id: clearMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: notifications.clear()
            }
        }

        StellarIcon {
            id: notificationIcon
            x: 34
            y: 12
            size: 28
            showTile: false
            iconName: notifications.current.appIcon !== undefined
                && notifications.current.appIcon.length > 0
                ? notifications.current.appIcon
                : "preferences-desktop-notification-bell"
        }

        Column {
            anchors.left: notificationIcon.right
            anchors.leftMargin: 10
            anchors.right: parent.right
            anchors.rightMargin: 14
            anchors.top: parent.top
            anchors.topMargin: 9
            spacing: 1

            Row {
                width: parent.width

                Text {
                    width: parent.width - timeText.implicitWidth - 6
                    text: notifications.current.appName !== undefined
                        && notifications.current.appName.length > 0
                        ? notifications.current.appName
                        : qsTr("Notificación")
                    color: Theme.textOnLightSecondary
                    font.family: Theme.fontFamily
                    font.pixelSize: 9
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }

                Text {
                    id: timeText
                    text: notifications.current.timestamp !== undefined
                        ? Qt.formatTime(new Date(notifications.current.timestamp), "hh:mm")
                        : ""
                    color: Theme.textOnLightSecondary
                    font.family: Theme.fontFamily
                    font.pixelSize: 9
                }
            }

            Text {
                width: parent.width
                text: notifications.current.summary !== undefined
                    ? notifications.current.summary
                    : ""
                color: Theme.textOnLight
                font.family: Theme.fontFamily
                font.pixelSize: 13
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }

            Text {
                width: parent.width
                text: notifications.current.body !== undefined
                    ? notifications.current.body
                    : ""
                textFormat: Text.PlainText
                color: Theme.textOnLightSecondary
                font.family: Theme.fontFamily
                font.pixelSize: 10
                wrapMode: Text.WordWrap
                maximumLineCount: 3
                elide: Text.ElideRight
            }
        }
    }
}
