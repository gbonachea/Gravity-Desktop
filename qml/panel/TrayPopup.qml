import QtQuick
import QtQuick.Controls
import "../common"
import "../common/Theme.js" as Theme

Item {
    id: root
    anchors.fill: parent

    GlassCard {
        id: card
        anchors.fill: parent
        radius: Theme.radius
        tintTop: Theme.menuTop
        tintBottom: Theme.menuBottom
        borderColor: Theme.menuBorder
        shadowOffset: 10

        MouseArea { anchors.fill: parent }

        // Title
        Text {
            id: title
            x: 20
            y: 16
            text: popup.currentType === "network" ? qsTr("Red y sonido")
                : popup.currentType === "volume" ? qsTr("Volumen")
                : popup.currentType === "calendar" ? qsTr("Calendario")
                : popup.currentType === "notifications" ? qsTr("Notificaciones")
                : qsTr("Batería")
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: 20
            font.weight: Font.DemiBold
        }

        GlassButton {
            x: card.width - width - 20
            y: 14
            width: 84
            height: 34
            text: qsTr("Cerrar")
            onClicked: popup.close()
        }

        // ============================== CALENDAR
        CalendarPopup {
            visible: popup.currentType === "calendar"
            x: 20
            y: 60
            width: card.width - 40
            height: card.height - 92
        }

        // ============================== NETWORK
        Column {
            visible: popup.currentType === "network"
            x: 20
            y: 52
            width: card.width - 40
            spacing: 8

            Text {
                text: system.networkManagerAvailable
                    ? qsTr("Conectividad: %1").arg(system.connectivity)
                    : qsTr("NetworkManager no disponible")
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: 11
            }

            // Wi-Fi toggle
            Rectangle {
                width: parent.width
                height: 44
                radius: 10
                color: "#14ffffff"
                border.width: 1
                border.color: "#26ffffff"

                Text {
                    x: 12
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Wi-Fi")
                    color: Theme.textPrimary
                    font.family: Theme.fontFamily
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                }

                Text {
                    x: 12
                    y: 24
                    text: system.wifiEnabled ? qsTr("Activado") : qsTr("Desactivado")
                    color: system.wifiEnabled ? "#9befb3" : Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: 11
                }

                Rectangle {
                    id: wifiSwitch
                    width: 42
                    height: 22
                    radius: 11
                    x: system.wifiEnabled ? parent.width - width - 6 : 6
                    anchors.verticalCenter: parent.verticalCenter
                    color: system.wifiEnabled ? "#9942c46e" : "#33ffffff"
                    border.width: 1
                    border.color: system.wifiEnabled ? "#aa9befb3" : "#33ffffff"
                    opacity: system.networkManagerAvailable ? 1.0 : 0.45

                    Behavior on x { NumberAnimation { duration: Theme.fast } }

                    Rectangle {
                        width: 16
                        height: 16
                        radius: 8
                        y: 3
                        x: system.wifiEnabled ? parent.width - width - 3 : 3
                        color: "#ffffff"
                        Behavior on x { NumberAnimation { duration: Theme.fast } }
                    }

                    MouseArea {
                        anchors.fill: parent
                        enabled: system.networkManagerAvailable
                        cursorShape: Qt.PointingHandCursor
                        onClicked: system.setWifiEnabled(!system.wifiEnabled)
                    }
                }
            }

            // Interfaces list
            Text {
                text: qsTr("INTERFACES")
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: 11
                font.letterSpacing: 1.0
                font.weight: Font.DemiBold
            }

            ScrollView {
                width: parent.width
                height: 84
                clip: true

                ListView {
                    width: parent.width
                    spacing: 2
                    model: system.interfaces

                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 28
                        radius: 8
                        color: "#0fffffff"

                        Text {
                            x: 8
                            anchors.verticalCenter: parent.verticalCenter
                            width: 150
                            text: modelData.name
                            color: Theme.textPrimary
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                        }

                        Text {
                            x: 144
                            anchors.verticalCenter: parent.verticalCenter
                            width: 110
                            text: modelData.type
                            color: Theme.textSecondary
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                            elide: Text.ElideRight
                        }

                        Text {
                            anchors.right: parent.right
                            anchors.rightMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData.state
                            color: modelData.connected ? "#9befb3" : Theme.textMuted
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                        }
                    }

                    Text {
                        anchors.centerIn: parent
                        visible: system.interfaces.length === 0
                        text: system.networkManagerAvailable
                            ? qsTr("Sin interfaces")
                            : qsTr("NetworkManager no disponible")
                        color: Theme.textMuted
                        font.family: Theme.fontFamily
                        font.pixelSize: 11
                    }
                }
            }

            // Wi-Fi networks header + scan
            Row {
                width: parent.width

                Text {
                    text: qsTr("REDES WI-FI")
                    color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: 11
                    font.letterSpacing: 1.0
                    font.weight: Font.DemiBold
                    anchors.verticalCenter: parent.verticalCenter
                }

                Item { width: parent.width - 120; height: 1 }

                GlassButton {
                    width: 112
                    height: 26
                    text: system.scanning ? qsTr("Escaneando…") : qsTr("Escanear")
                    onClicked: system.scanWifi()
                }
            }

            ScrollView {
                width: parent.width
                height: 210
                clip: true

                ListView {
                    id: wifiList
                    width: parent.width
                    spacing: 2
                    model: system.wifiNetworks

                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 48
                        radius: 8
                        color: wifiMouse.containsMouse ? "#1affffff" : "#0fffffff"
                        border.width: 1
                        border.color: "#18ffffff"

                        MouseArea {
                            id: wifiMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (modelData.secured) {
                                    passwordDialog.ssid = modelData.ssid
                                    passwordDialog.open()
                                } else {
                                    system.connectToWifi(modelData.ssid, "")
                                }
                            }
                        }

                        Column {
                            anchors.left: parent.left
                            anchors.leftMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 1

                            Text {
                                width: 290
                                text: modelData.ssid
                                color: Theme.textPrimary
                                font.family: Theme.fontFamily
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                                elide: Text.ElideRight
                            }

                            Text {
                                text: modelData.secured
                                    ? modelData.security + " · " + modelData.band
                                    : qsTr("Abierta") + " · " + modelData.band
                                color: Theme.textMuted
                                font.family: Theme.fontFamily
                                font.pixelSize: 11
                            }
                        }

                        Text {
                            anchors.right: parent.right
                            anchors.rightMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData.strength + "%"
                            color: Theme.textSecondary
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                        }
                    }

                    Text {
                        anchors.centerIn: parent
                        visible: system.wifiNetworks.length === 0
                        text: system.scanning
                            ? qsTr("Escaneando redes…")
                            : (system.networkManagerAvailable
                                ? qsTr("Sin resultados. Pulsa Escanear.")
                                : qsTr("NetworkManager no disponible"))
                        color: Theme.textMuted
                        font.family: Theme.fontFamily
                        font.pixelSize: 11
                    }
                }
            }
        }

        // ============================== VOLUME
        Column {
            visible: popup.currentType === "volume"
            x: 20
            y: 52
            width: card.width - 40
            spacing: 12

            Text {
                text: system.muted ? qsTr("Silenciado") : system.volume + "%"
                color: system.muted ? "#ffb3bd" : Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: 30
                font.weight: Font.DemiBold
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Item {
                id: volumeBar
                width: parent.width
                height: 12

                Rectangle {
                    anchors.fill: parent
                    radius: 6
                    color: "#26ffffff"
                }

                Rectangle {
                    width: parent.width * Math.max(0, system.volume) / 100
                    height: parent.height
                    radius: 6
                    opacity: system.muted ? 0.45 : 1.0
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: "#59aaff" }
                        GradientStop { position: 1.0; color: "#2a7cf0" }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    anchors.topMargin: -8
                    anchors.bottomMargin: -8
                    cursorShape: Qt.PointingHandCursor
                    onPressed: function(mouse) { setVolumeFromMouse(mouse.x) }
                    onPositionChanged: function(mouse) { if (pressed) setVolumeFromMouse(mouse.x) }
                    function setVolumeFromMouse(x) {
                        if (volumeBar.width <= 0) return
                        var v = Math.round(Theme.clamp(x / volumeBar.width, 0, 1) * 100)
                        system.setVolume(v)
                    }
                }
            }

            Row {
                spacing: 6
                anchors.horizontalCenter: parent.horizontalCenter

                GlassButton {
                    width: 150
                    height: 34
                    text: system.muted ? qsTr("Activar sonido") : qsTr("Silenciar")
                    onClicked: system.toggleMute()
                }

                GlassButton {
                    width: 60
                    height: 34
                    text: "−"
                    onClicked: system.decreaseVolume()
                }

                GlassButton {
                    width: 60
                    height: 34
                    text: "+"
                    onClicked: system.increaseVolume()
                }
            }

            Text {
                text: qsTr("Usa la rueda del ratón sobre el icono de volumen para ajustar.")
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: 11
                wrapMode: Text.WordWrap
                width: parent.width
            }
        }

        // ============================== BATTERY
        Column {
            visible: popup.currentType === "battery"
            x: 20
            y: 52
            width: card.width - 40
            spacing: 12

            Row {
                spacing: 12
                anchors.horizontalCenter: parent.horizontalCenter

                BatteryGlyph {
                    width: 40
                    height: 20
                    level: tray.batteryLevel
                    charging: tray.charging
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: tray.batteryLevel >= 0 ? tray.batteryLevel + "%" : qsTr("Sin batería")
                    color: Theme.textPrimary
                    font.family: Theme.fontFamily
                    font.pixelSize: 30
                    font.weight: Font.DemiBold
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            Text {
                text: tray.charging ? qsTr("Cargando…") : qsTr("Descargando")
                color: tray.charging ? "#9befb3" : Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: 13
                anchors.horizontalCenter: parent.horizontalCenter
            }

            Rectangle {
                width: parent.width
                height: 10
                radius: 5
                color: "#26ffffff"

                Rectangle {
                    width: parent.width * Math.max(0, tray.batteryLevel) / 100
                    height: parent.height
                    radius: 5
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: tray.charging ? "#42c46e" : "#59aaff" }
                        GradientStop { position: 1.0; color: tray.charging ? "#2a9d55" : "#2a7cf0" }
                    }
                }
            }

            Text {
                text: qsTr("Estado: %1").arg(tray.charging ? qsTr("En carga") : qsTr("En uso"))
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: 11
            }

            Text {
                text: qsTr("Nivel actual: %1%").arg(tray.batteryLevel >= 0 ? tray.batteryLevel : 0)
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: 11
            }

            Text {
                visible: !tray.hasBattery
                text: qsTr("No se detectó batería en este equipo.")
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: 11
            }
        }

        // ============================== NOTIFICATIONS
        Column {
            visible: popup.currentType === "notifications"
            x: 20
            y: 52
            width: card.width - 40
            spacing: 8

            Row {
                width: parent.width
                height: 30

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: notifications.count > 0
                        ? qsTr("%1 notificaciones").arg(notifications.count)
                        : qsTr("No hay notificaciones")
                    color: Theme.textSecondary
                    font.family: Theme.fontFamily
                    font.pixelSize: 11
                }

                Item { width: parent.width - 170; height: 1 }

                GlassButton {
                    width: 90
                    height: 30
                    text: qsTr("Limpiar")
                    enabled: notifications.count > 0
                    onClicked: notifications.clear()
                }
            }

            ListView {
                width: parent.width
                height: card.height - 138
                clip: true
                spacing: 6
                model: notifications.items

                delegate: Rectangle {
                    width: ListView.view.width
                    height: Math.max(74, notificationColumn.implicitHeight + 22)
                    radius: 10
                    color: "#10ffffff"
                    border.width: 1
                    border.color: "#1fffffff"

                    StellarIcon {
                        id: notificationIcon
                        x: 10
                        y: 10
                        size: 28
                        showTile: false
                        iconName: modelData.appIcon && modelData.appIcon.length > 0
                            ? modelData.appIcon
                            : "preferences-desktop-notification-bell"
                    }

                    Column {
                        id: notificationColumn
                        anchors.left: notificationIcon.right
                        anchors.leftMargin: 10
                        anchors.right: parent.right
                        anchors.rightMargin: 30
                        anchors.top: parent.top
                        anchors.topMargin: 9
                        spacing: 1

                        Text {
                            width: parent.width
                            text: modelData.appName && modelData.appName.length > 0
                                ? modelData.appName
                                : qsTr("Notificación")
                            color: Theme.textMuted
                            font.family: Theme.fontFamily
                            font.pixelSize: 9
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                        }

                        Text {
                            width: parent.width
                            text: modelData.summary
                            color: Theme.textPrimary
                            font.family: Theme.fontFamily
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                            elide: Text.ElideRight
                        }

                        Text {
                            width: parent.width
                            text: modelData.body
                            textFormat: Text.PlainText
                            color: Theme.textSecondary
                            font.family: Theme.fontFamily
                            font.pixelSize: 10
                            wrapMode: Text.WordWrap
                            maximumLineCount: 2
                            elide: Text.ElideRight
                        }
                    }

                    Text {
                        anchors.right: parent.right
                        anchors.rightMargin: 30
                        anchors.top: parent.top
                        anchors.topMargin: 9
                        text: Qt.formatTime(new Date(modelData.timestamp), "hh:mm")
                        color: Theme.textMuted
                        font.family: Theme.fontFamily
                        font.pixelSize: 9
                    }

                    Item {
                        width: 22
                        height: 22
                        anchors.right: parent.right
                        anchors.rightMargin: 5
                        anchors.top: parent.top
                        anchors.topMargin: 4

                        Rectangle {
                            anchors.fill: parent
                            radius: 11
                            color: dismissMouse.containsMouse ? "#33ffffff" : "transparent"
                        }

                        Text {
                            anchors.centerIn: parent
                            text: "✕"
                            color: Theme.textSecondary
                            font.pixelSize: 10
                            font.family: Theme.fontFamily
                        }

                        MouseArea {
                            id: dismissMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: notifications.dismiss(modelData.id)
                        }
                    }
                }

                Text {
                    anchors.centerIn: parent
                    visible: notifications.count === 0
                    text: qsTr("No hay notificaciones")
                    color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: 11
                }
            }
        }
    }

    // Password dialog for secured Wi-Fi
    Rectangle {
        id: passwordDialog
        property string ssid: ""
        property bool opened: false

        anchors.fill: parent
        color: "#88000000"
        visible: opened
        z: 100

        function open() { opened = true; passwordField.text = ""; passwordField.forceActiveFocus() }
        function close() { opened = false }

        MouseArea { anchors.fill: parent; onClicked: passwordDialog.close() }

        GlassCard {
            width: 380
            height: 190
            anchors.centerIn: parent
            radius: Theme.radius
            tintTop: Theme.menuTop
            tintBottom: Theme.menuBottom
            borderColor: Theme.menuBorder
            shadowOffset: 10

            MouseArea { anchors.fill: parent }

            Text {
                x: 20
                y: 16
                text: qsTr("Contraseña para %1").arg(passwordDialog.ssid)
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: 15
                font.weight: Font.DemiBold
                width: parent.width - 40
                elide: Text.ElideRight
            }

            TextField {
                id: passwordField
                x: 20
                y: 52
                width: parent.width - 40
                height: 34
                echoMode: TextInput.Password
                placeholderText: qsTr("Contraseña")
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: 13
                background: Rectangle {
                    radius: 8
                    color: "#1affffff"
                    border.width: 1
                    border.color: "#33ffffff"
                }
            }

            Row {
                x: 20
                y: 100
                spacing: 8

                GlassButton {
                    width: 140
                    height: 34
                    text: qsTr("Conectar")
                    onClicked: {
                        system.connectToWifi(passwordDialog.ssid, passwordField.text)
                        passwordDialog.close()
                    }
                }

                GlassButton {
                    width: 140
                    height: 34
                    text: qsTr("Cancelar")
                    onClicked: passwordDialog.close()
                }
            }
        }
    }
}