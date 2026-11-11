import QtQuick
import "../common"
import "../common/Theme.js" as Theme

Item {
    id: root

    width: 1920
    height: Theme.panelHeight

    // Clicking any empty part of the panel (except the Start button) dismisses
    // an open start menu. Excluding the Start button area prevents a stray
    // "hide" from racing with the button's own "toggle" command.
    MouseArea {
        anchors.left: startButton.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        z: -1
        onClicked: menuClient.hideMenu()
    }

    GlassCard {
        id: bar
        anchors.fill: parent
        radius: Theme.radius
        tintTop: Theme.panelTop
        tintBottom: Theme.panelBottom
        borderColor: Theme.panelBorder
        shadowOffset: 5
        opacity: settings ? Math.max(0.3, settings.panelOpacity / 100) : 1.0
    }

    StartButton {
        id: startButton
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        onClicked: menuClient.toggle()
    }

    Rectangle {
        id: leftDivider
        anchors.left: startButton.right
        anchors.leftMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        width: 1
        height: 30
        color: Theme.divider
    }

    ListView {
        id: taskList
        anchors.left: leftDivider.right
        anchors.leftMargin: 12
        anchors.right: trayRow.left
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        height: 46
        orientation: ListView.Horizontal
        spacing: 8
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        model: toplevels

        delegate: TaskButton {
            width: model.active ? 178 : 46
            title: model.title.length > 0 ? model.title : (model.appId.length > 0 ? model.appId : qsTr("Sin título"))
            iconName: model.icon
            active: model.active
            minimized: model.minimized
            onActivated: toplevels.toggleMinimize(index)
            onCloseRequested: toplevels.closeWindow(index)
        }


    }

    Row {
        id: trayRow
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2

        Rectangle {
            width: 38
            height: 46
            radius: 10
            color: wifiMouse.containsMouse ? Theme.hover : "transparent"

            WifiGlyph {
                anchors.centerIn: parent
                width: 22
                height: 22
                online: tray.online
            }

            MouseArea {
                id: wifiMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    menuClient.hideMenu()
                    popup.show("network")
                }
            }

            Rectangle {
                visible: !tray.online
                width: 6
                height: 6
                radius: 3
                color: "#ff8b98"
                anchors.right: parent.right
                anchors.rightMargin: 7
                anchors.top: parent.top
                anchors.topMargin: 10
            }
        }

        Rectangle {
            width: 38
            height: 46
            radius: 10
            color: volumeMouse.containsMouse ? Theme.hover : "transparent"

            VolumeGlyph {
                anchors.centerIn: parent
                width: 22
                height: 22
                volume: system.muted ? 0 : system.volume
            }

            MouseArea {
                id: volumeMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    menuClient.hideMenu()
                    popup.show("volume")
                }
            }

            WheelHandler {
                onWheel: function(event) {
                    if (event.angleDelta.y > 0)
                        system.increaseVolume()
                    else
                        system.decreaseVolume()
                }
            }
        }

        Rectangle {
            visible: tray.hasBattery
            width: 54
            height: 46
            radius: 10
            color: batteryMouse.containsMouse ? Theme.hover : "transparent"

            Row {
                anchors.centerIn: parent
                spacing: 4

                BatteryGlyph {
                    width: 26
                    height: 14
                    level: tray.batteryLevel
                    charging: tray.charging
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: tray.batteryLevel >= 0 ? tray.batteryLevel + "%" : ""
                    color: Theme.textSecondary
                    font.family: Theme.fontFamily
                    font.pixelSize: 10
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            MouseArea {
                id: batteryMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    menuClient.hideMenu()
                    popup.show("battery")
                }
            }
        }

        Rectangle {
            id: notificationButton
            visible: settings.notificationsTrayIcon
            width: 38
            height: 46
            radius: 10
            color: notificationMouse.containsMouse ? Theme.hover : "transparent"

            StellarIcon {
                anchors.centerIn: parent
                size: 22
                showTile: false
                iconName: "preferences-desktop-notification-bell"
            }

            Rectangle {
                visible: notifications.count > 0
                width: Math.max(14, badgeText.implicitWidth + 6)
                height: 14
                radius: 7
                color: "#e64455"
                border.width: 1
                border.color: "#66ffd7dc"
                anchors.right: parent.right
                anchors.rightMargin: 3
                anchors.top: parent.top
                anchors.topMargin: 5

                Text {
                    id: badgeText
                    anchors.centerIn: parent
                    text: notifications.count > 99 ? "99+" : String(notifications.count)
                    color: "#ffffff"
                    font.family: Theme.fontFamily
                    font.pixelSize: 8
                    font.weight: Font.Bold
                }
            }

            MouseArea {
                id: notificationMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    menuClient.hideMenu()
                    popup.show("notifications")
                }
            }
        }

        Rectangle {
            width: 1
            height: 28
            color: Theme.divider
            anchors.verticalCenter: parent.verticalCenter
        }

        Rectangle {
            id: clockArea
            width: clockRow.implicitWidth + 22
            height: 46
            radius: 10
            color: clockMouse.containsMouse ? Theme.hover : "transparent"

            Row {
                id: clockRow
                anchors.centerIn: parent
                spacing: 4

                Text {
                    text: clock.time
                    color: Theme.textPrimary
                    font.family: Theme.fontFamily
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    visible: clock.period.length > 0
                    text: clock.period
                    color: Theme.textSecondary
                    font.family: Theme.fontFamily
                    font.pixelSize: 10
                    font.weight: Font.DemiBold
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.verticalCenterOffset: -1
                }
            }

            MouseArea {
                id: clockMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    menuClient.hideMenu()
                    popup.show("calendar")
                }
            }
        }
    }

    // Transient feedback for native tray actions (volume, Wi-Fi scan…).
    Rectangle {
        id: trayToast
        anchors.right: trayRow.left
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        width: Math.min(280, trayToastText.implicitWidth + 24)
        height: 34
        radius: 10
        color: "#e60b2452"
        border.width: 1
        border.color: "#66cfe7ff"
        opacity: 0.0
        visible: opacity > 0.01
        z: 30

        function show(message) {
            trayToastText.text = message
            trayToast.opacity = 1.0
            trayToastTimer.restart()
        }

        Text {
            id: trayToastText
            anchors.centerIn: parent
            width: parent.width - 20
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
            color: "#e8f3ff"
            font.family: Theme.fontFamily
            font.pixelSize: 11
            font.weight: Font.DemiBold
        }

        Timer {
            id: trayToastTimer
            interval: 2600
            onTriggered: trayToast.opacity = 0.0
        }

        Behavior on opacity { NumberAnimation { duration: Theme.fast } }
    }

    Connections {
        target: system

        function onErrorOccurred(message) {
            trayToast.show(message)
        }

        function onScanningChanged() {
            if (system.scanning) {
                return
            }
            if (system.wifiNetworks.length > 0) {
                trayToast.show(qsTr("%1 redes Wi-Fi encontradas").arg(system.wifiNetworks.length))
            } else {
                trayToast.show(qsTr("No se encontraron redes Wi-Fi"))
            }
        }

        function onWifiEnabledChanged() {
            trayToast.show(system.wifiEnabled ? qsTr("Wi-Fi activado") : qsTr("Wi-Fi desactivado"))
        }

        function onMutedChanged() {
            trayToast.show(system.muted ? qsTr("Silenciado") : qsTr("Volumen %1%").arg(system.volume))
        }

        function onNetworkManagerAvailableChanged() {
            if (!system.networkManagerAvailable)
                trayToast.show(qsTr("NetworkManager no disponible"))
        }
    }
}
