import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../common"
import "../common/Theme.js" as Theme

Item {
    id: root
    width: 1040
    height: 700

    property alias currentPage: sidebar.currentIndex

    GlassCard {
        anchors.fill: parent
        radius: Theme.radius
        tintTop: Theme.menuTop
        tintBottom: Theme.menuBottom
        borderColor: Theme.menuBorder
        shadowOffset: 10
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        // ------------------------------------------------------------------
        // Sidebar
        // ------------------------------------------------------------------
        Rectangle {
            Layout.preferredWidth: 250
            Layout.fillHeight: true
            radius: 14
            color: "#14ffffff"
            border.width: 1
            border.color: "#26ffffff"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 6

                Text {
                    text: qsTr("Centro de configuración")
                    color: Theme.textPrimary
                    font.family: Theme.fontFamily
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                ListView {
                    id: sidebar
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 2
                    model: [
                        { icon: "preferences-system", label: qsTr("Panel") },
                        { icon: "applications-system", label: qsTr("Menú") },
                        { icon: "applications-graphics", label: qsTr("Widgets") },
                        { icon: "audio-volume-high", label: qsTr("Volumen") },
                        { icon: "network-wireless", label: qsTr("Red") },
                        { icon: "battery", label: qsTr("Energía") },
                        { icon: "preferences-system-time", label: qsTr("Fecha y hora") },
                        { icon: "input-keyboard", label: qsTr("Teclado") },
                        { icon: "input-mouse", label: qsTr("Ratón") },
                        { icon: "input-touchpad", label: qsTr("Touchpad") },
                        { icon: "preferences-desktop-theme", label: qsTr("Apariencia") },
                        { icon: "video-display", label: qsTr("Pantalla") },
                        { icon: "dialog-information", label: qsTr("Sistema") },
                        { icon: "preferences-system", label: qsTr("Dispositivos") },
                        { icon: "preferences-desktop-default-applications",
                          label: qsTr("Apps predeterminadas"),
                          title: qsTr("Aplicaciones predeterminadas") }
                    ]
                    currentIndex: 0

                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 42
                        radius: 10
                        color: sidebar.currentIndex === index
                            ? "#33ffffff"
                            : (itemMouse.containsMouse ? "#1affffff" : "transparent")

                        Row {
                            anchors.left: parent.left
                            anchors.leftMargin: 12
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 10

                            StellarIcon {
                                iconName: modelData.icon
                                size: 22
                                showTile: false
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            Text {
                                text: modelData.label
                                color: Theme.textPrimary
                                font.family: Theme.fontFamily
                                font.pixelSize: 14
                                font.weight: sidebar.currentIndex === index
                                    ? Font.DemiBold : Font.Normal
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }

                        MouseArea {
                            id: itemMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: sidebar.currentIndex = index
                        }
                    }
                }
            }
        }

        // ------------------------------------------------------------------
        // Content
        // ------------------------------------------------------------------
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Text {
                    text: sidebar.model[sidebar.currentIndex].title !== undefined
                        ? sidebar.model[sidebar.currentIndex].title
                        : sidebar.model[sidebar.currentIndex].label
                    color: Theme.textPrimary
                    font.family: Theme.fontFamily
                    font.pixelSize: 24
                    font.weight: Font.DemiBold
                    Layout.fillWidth: true
                }

                Text {
                    id: applyStatus
                    text: ""
                    color: "#9befb3"
                    font.family: Theme.fontFamily
                    font.pixelSize: 12
                }

                GlassButton {
                    text: qsTr("Aplicar")
                    width: 130
                    height: 38
                    onClicked: {
                        settings.applyAll()
                        applyStatus.text = qsTr("Cambios aplicados")
                        applyTimer.restart()
                    }
                }
            }

            Timer {
                id: applyTimer
                interval: 2500
                onTriggered: applyStatus.text = ""
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#33ffffff"
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: sidebar.currentIndex

                // ==========================================================
                // 0 PANEL
                // ==========================================================
                ScrollView {
                    clip: true
                    Column {
                        width: parent.width
                        spacing: 18

                        Text { text: qsTr("Aspecto y comportamiento del panel inferior"); color: Theme.textSecondary; font.pixelSize: 13; font.family: Theme.fontFamily; wrapMode: Text.WordWrap; width: parent.width }

                        Row {
                            spacing: 14
                            Text { text: qsTr("Altura"); color: Theme.textPrimary; font.pixelSize: 14; font.family: Theme.fontFamily; anchors.verticalCenter: parent.verticalCenter; width: 160 }
                            Slider { width: 320; from: 48; to: 96; stepSize: 4; value: settings.panelHeight; onMoved: settings.panelHeight = Math.round(value) }
                            Text { text: settings.panelHeight + " px"; color: Theme.textSecondary; font.pixelSize: 13; font.family: Theme.fontFamily; anchors.verticalCenter: parent.verticalCenter }
                        }

                        Row {
                            spacing: 14
                            Text { text: qsTr("Posición"); color: Theme.textPrimary; font.pixelSize: 14; font.family: Theme.fontFamily; anchors.verticalCenter: parent.verticalCenter; width: 160 }
                            ComboBox {
                                width: 200
                                model: [qsTr("Abajo"), qsTr("Arriba")]
                                currentIndex: settings.panelPosition === "top" ? 1 : 0
                                onActivated: settings.panelPosition = index === 1 ? "top" : "bottom"
                            }
                        }

                        Row {
                            spacing: 14
                            Text { text: qsTr("Opacidad"); color: Theme.textPrimary; font.pixelSize: 14; font.family: Theme.fontFamily; anchors.verticalCenter: parent.verticalCenter; width: 160 }
                            Slider { width: 320; from: 30; to: 100; value: settings.panelOpacity; onMoved: settings.panelOpacity = Math.round(value) }
                            Text { text: settings.panelOpacity + "%"; color: Theme.textSecondary; font.pixelSize: 13; font.family: Theme.fontFamily; anchors.verticalCenter: parent.verticalCenter }
                        }

                        Row {
                            spacing: 14
                            Text { text: qsTr("Ocultar automáticamente"); color: Theme.textPrimary; font.pixelSize: 14; font.family: Theme.fontFamily; anchors.verticalCenter: parent.verticalCenter; width: 220 }
                            Switch { checked: settings.panelAutoHide; onToggled: settings.panelAutoHide = checked }
                        }

                        Text { text: qsTr("Los cambios de altura y posición se aplican al reiniciar el panel."); color: Theme.textMuted; font.pixelSize: 11; font.family: Theme.fontFamily; wrapMode: Text.WordWrap; width: parent.width }
                    }
                }

                // ==========================================================
                // 1 MENÚ
                // ==========================================================
                ScrollView {
                    clip: true
                    Column {
                        width: parent.width
                        spacing: 18

                        Text { text: qsTr("Opciones del menú Inicio"); color: Theme.textSecondary; font.pixelSize: 13; font.family: Theme.fontFamily; wrapMode: Text.WordWrap; width: parent.width }

                        Row {
                            spacing: 14
                            Text { text: qsTr("Mostrar buscador"); color: Theme.textPrimary; font.pixelSize: 14; font.family: Theme.fontFamily; anchors.verticalCenter: parent.verticalCenter; width: 220 }
                            Switch { checked: settings.menuShowSearch; onToggled: settings.menuShowSearch = checked }
                        }

                        Row {
                            spacing: 14
                            Text { text: qsTr("Mostrar aplicaciones recientes"); color: Theme.textPrimary; font.pixelSize: 14; font.family: Theme.fontFamily; anchors.verticalCenter: parent.verticalCenter; width: 220 }
                            Switch { checked: settings.menuShowRecent; onToggled: settings.menuShowRecent = checked }
                        }

                        Rectangle { width: parent.width; height: 1; color: "#33ffffff" }

                        Text { text: qsTr("Aplicaciones favoritas"); color: Theme.textMuted; font.pixelSize: 12; font.family: Theme.fontFamily; font.weight: Font.DemiBold }
                        Text {
                            text: qsTr("Son las que aparecen en la columna «APLICACIONES» del menú Inicio. Si no eliges ninguna, la lista queda vacía.")
                            color: Theme.textSecondary
                            font.pixelSize: 12
                            font.family: Theme.fontFamily
                            wrapMode: Text.WordWrap
                            width: parent.width
                        }

                        ListView {
                            width: parent.width
                            height: Math.min(220, Math.max(46, settings.menuFavorites.length * 42))
                            clip: true
                            spacing: 4
                            model: settings.menuFavorites
                            delegate: Rectangle {
                                width: ListView.view.width
                                height: 38
                                radius: 8
                                color: "#14ffffff"
                                border.width: 1
                                border.color: "#22ffffff"

                                Row {
                                    anchors.left: parent.left
                                    anchors.leftMargin: 12
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 8
                                    StellarIcon { iconName: settings.appIcon(modelData); size: 24; anchors.verticalCenter: parent.verticalCenter }
                                    Text {
                                        text: settings.appName(modelData)
                                        color: Theme.textPrimary
                                        font.pixelSize: 13
                                        font.family: Theme.fontFamily
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }

                                Row {
                                    anchors.right: parent.right
                                    anchors.rightMargin: 8
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 4
                                    GlassButton { width: 36; height: 30; text: "↑"; onClicked: settings.moveFavorite(modelData, -1) }
                                    GlassButton { width: 36; height: 30; text: "↓"; onClicked: settings.moveFavorite(modelData, 1) }
                                    GlassButton { width: 36; height: 30; text: "✕"; onClicked: settings.toggleFavorite(modelData) }
                                }
                            }

                            Text {
                                anchors.centerIn: parent
                                visible: settings.menuFavorites.length === 0
                                text: qsTr("Sin favoritos: la lista de Aplicativos estará vacía")
                                color: Theme.textMuted
                                font.pixelSize: 12
                                font.family: Theme.fontFamily
                            }
                        }

                        Rectangle { width: parent.width; height: 1; color: "#33ffffff" }

                        Text { text: qsTr("Todas las aplicaciones"); color: Theme.textMuted; font.pixelSize: 12; font.family: Theme.fontFamily; font.weight: Font.DemiBold }

                        ListView {
                            width: parent.width
                            height: 320
                            clip: true
                            spacing: 3
                            model: settings.appChoices
                            delegate: Rectangle {
                                width: ListView.view.width
                                height: 34
                                radius: 8
                                color: "#0fffffff"
                                Row {
                                    anchors.left: parent.left
                                    anchors.leftMargin: 10
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 8
                                    StellarIcon { iconName: modelData.icon; size: 20; anchors.verticalCenter: parent.verticalCenter }
                                    Text {
                                        text: modelData.name
                                        color: Theme.textPrimary
                                        font.pixelSize: 12
                                        font.family: Theme.fontFamily
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }
                                Switch {
                                    anchors.right: parent.right
                                    anchors.rightMargin: 8
                                    anchors.verticalCenter: parent.verticalCenter
                                    checked: modelData.favorite
                                    onToggled: settings.toggleFavorite(modelData.file)
                                }
                            }
                        }

                        Text { text: qsTr("El menú muestra aplicaciones, accesos rápidos, energía y el acceso al centro de configuración."); color: Theme.textMuted; font.pixelSize: 11; font.family: Theme.fontFamily; wrapMode: Text.WordWrap; width: parent.width }
                    }
                }

                // ==========================================================
                // 2 WIDGETS
                // ==========================================================
                ScrollView {
                    clip: true
                    Column {
                        width: parent.width
                        spacing: 14

                        Text { text: qsTr("Elige qué widgets se muestran y su orden"); color: Theme.textSecondary; font.pixelSize: 13; font.family: Theme.fontFamily; wrapMode: Text.WordWrap; width: parent.width }

                        ListView {
                            width: parent.width
                            height: 3 * 56
                            clip: true
                            spacing: 6
                            model: settings.widgetOrder

                            delegate: Rectangle {
                                width: ListView.view.width
                                height: 50
                                radius: 10
                                color: "#14ffffff"
                                border.width: 1
                                border.color: "#22ffffff"

                                Row {
                                    anchors.left: parent.left
                                    anchors.leftMargin: 12
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 10

                                    Switch {
                                        checked: modelData === "clock" ? settings.widgetClock
                                            : modelData === "storage" ? settings.widgetStorage
                                            : settings.widgetStatus
                                        onToggled: {
                                            if (modelData === "clock") settings.widgetClock = checked
                                            else if (modelData === "storage") settings.widgetStorage = checked
                                            else settings.widgetStatus = checked
                                        }
                                    }

                                    Text {
                                        text: settings.widgetLabel(modelData)
                                        color: Theme.textPrimary
                                        font.pixelSize: 14
                                        font.family: Theme.fontFamily
                                        anchors.verticalCenter: parent.verticalCenter
                                    }
                                }

                                Row {
                                    anchors.right: parent.right
                                    anchors.rightMargin: 10
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 6

                                    GlassButton { width: 40; height: 32; text: "↑"; onClicked: settings.moveWidget(modelData, -1) }
                                    GlassButton { width: 40; height: 32; text: "↓"; onClicked: settings.moveWidget(modelData, 1) }
                                }
                            }
                        }

                        Rectangle { width: parent.width; height: 1; color: "#33ffffff" }

                        Text { text: qsTr("Clima del widget de reloj"); color: Theme.textMuted; font.pixelSize: 12; font.family: Theme.fontFamily; font.weight: Font.DemiBold }

                        Row {
                            spacing: 14
                            Text { text: qsTr("Ubicación automática por red"); color: Theme.textPrimary; font.pixelSize: 14; font.family: Theme.fontFamily; anchors.verticalCenter: parent.verticalCenter; width: 240 }
                            Switch {
                                checked: settings.weatherAutoLocate
                                onToggled: settings.weatherAutoLocate = checked
                            }
                        }

                        Row {
                            spacing: 10
                            TextField {
                                id: weatherCityField
                                width: 280
                                height: 38
                                placeholderText: qsTr("Ciudad (ej.: Madrid)")
                                color: Theme.textPrimary
                                font.pixelSize: 13
                                font.family: Theme.fontFamily
                                background: Rectangle { radius: 10; color: "#1affffff"; border.width: 1; border.color: "#33ffffff" }
                                Component.onCompleted: text = settings.weatherCity
                                onEditingFinished: settings.weatherCity = text
                            }
                            GlassButton {
                                text: qsTr("Usar ciudad")
                                width: 130
                                height: 38
                                onClicked: {
                                    settings.weatherCity = weatherCityField.text
                                    settings.weatherAutoLocate = false
                                }
                            }
                        }

                        Text { text: qsTr("El clima se actualiza al reiniciar stellar-widgets o al pulsar Aplicar. Si la ubicación automática falla, se usa la ciudad indicada."); color: Theme.textMuted; font.pixelSize: 11; font.family: Theme.fontFamily; wrapMode: Text.WordWrap; width: parent.width }

                        Text { text: qsTr("Los cambios se aplican en caliente al reiniciar stellar-widgets."); color: Theme.textMuted; font.pixelSize: 11; font.family: Theme.fontFamily; wrapMode: Text.WordWrap; width: parent.width }
                    }
                }

                // ==========================================================
                // 3 VOLUMEN
                // ==========================================================
                ScrollView {
                    clip: true
                    Column {
                        width: parent.width
                        spacing: 18

                        Text { text: qsTr("Salida de audio predeterminada"); color: Theme.textSecondary; font.pixelSize: 13; font.family: Theme.fontFamily }

                        Text {
                            text: system.muted ? qsTr("Silenciado") : system.volume + "%"
                            color: Theme.textPrimary
                            font.pixelSize: 32
                            font.weight: Font.DemiBold
                            font.family: Theme.fontFamily
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        Slider {
                            width: parent.width
                            from: 0; to: 100
                            value: system.volume
                            onMoved: system.setVolume(Math.round(value))
                        }

                        Row {
                            spacing: 10
                            anchors.horizontalCenter: parent.horizontalCenter
                            GlassButton { text: system.muted ? qsTr("Activar sonido") : qsTr("Silenciar"); width: 170; height: 38; onClicked: system.toggleMute() }
                            GlassButton { text: "−"; width: 60; height: 38; onClicked: system.decreaseVolume() }
                            GlassButton { text: "+"; width: 60; height: 38; onClicked: system.increaseVolume() }
                        }
                    }
                }

                // ==========================================================
                // 4 RED
                // ==========================================================
                ScrollView {
                    clip: true
                    Column {
                        width: parent.width
                        spacing: 14

                        Text {
                            text: system.networkManagerAvailable
                                ? qsTr("Conectividad: %1").arg(system.connectivity)
                                : qsTr("NetworkManager no disponible")
                            color: Theme.textSecondary
                            font.pixelSize: 13
                            font.family: Theme.fontFamily
                        }

                        Row {
                            spacing: 10
                            GlassButton { text: system.wifiEnabled ? qsTr("Desactivar Wi-Fi") : qsTr("Activar Wi-Fi"); width: 180; height: 38; enabled: system.networkManagerAvailable; onClicked: system.toggleWifi() }
                            GlassButton { text: system.scanning ? qsTr("Escaneando…") : qsTr("Escanear"); width: 140; height: 38; enabled: system.networkManagerAvailable; onClicked: system.scanWifi() }
                        }

                        Text { text: qsTr("Interfaces"); color: Theme.textMuted; font.pixelSize: 12; font.family: Theme.fontFamily; font.weight: Font.DemiBold }

                        ListView {
                            width: parent.width
                            height: 100
                            clip: true
                            spacing: 3
                            model: system.interfaces
                            delegate: Rectangle {
                                width: ListView.view.width
                                height: 28
                                radius: 8
                                color: "#0fffffff"
                                Text { x: 10; anchors.verticalCenter: parent.verticalCenter; text: modelData.name + "  (" + modelData.type + ")"; color: Theme.textPrimary; font.pixelSize: 12; font.family: Theme.fontFamily }
                                Text { anchors.right: parent.right; anchors.rightMargin: 10; anchors.verticalCenter: parent.verticalCenter; text: modelData.state; color: modelData.connected ? "#9befb3" : Theme.textMuted; font.pixelSize: 11; font.family: Theme.fontFamily }
                            }
                        }

                        Text { text: qsTr("Redes Wi-Fi"); color: Theme.textMuted; font.pixelSize: 12; font.family: Theme.fontFamily; font.weight: Font.DemiBold }

                        ListView {
                            width: parent.width
                            height: 180
                            clip: true
                            spacing: 3
                            model: system.wifiNetworks
                            delegate: Rectangle {
                                width: ListView.view.width
                                height: 34
                                radius: 8
                                color: "#0fffffff"
                                Text { x: 10; anchors.verticalCenter: parent.verticalCenter; text: modelData.ssid; color: Theme.textPrimary; font.pixelSize: 12; font.family: Theme.fontFamily }
                                Text { anchors.right: parent.right; anchors.rightMargin: 10; anchors.verticalCenter: parent.verticalCenter; text: modelData.strength + "%"; color: Theme.textSecondary; font.pixelSize: 11; font.family: Theme.fontFamily }
                            }
                        }
                    }
                }

                // ==========================================================
                // 5 ENERGÍA
                // ==========================================================
                ScrollView {
                    clip: true
                    Column {
                        width: parent.width
                        spacing: 16

                        Text { text: tray.hasBattery ? qsTr("Batería: %1%%2").arg(tray.batteryLevel).arg(tray.charging ? qsTr(" (cargando)") : qsTr(" (descargando)")) : qsTr("No se detectó batería"); color: Theme.textSecondary; font.pixelSize: 14; font.family: Theme.fontFamily }

                        Grid {
                            columns: 2
                            spacing: 10
                            GlassButton { text: qsTr("Apagar"); width: 180; height: 38; enabled: system.canPowerOff; onClicked: system.powerOff() }
                            GlassButton { text: qsTr("Reiniciar"); width: 180; height: 38; enabled: system.canReboot; onClicked: system.reboot() }
                            GlassButton { text: qsTr("Suspender"); width: 180; height: 38; enabled: system.canSuspend; onClicked: system.suspend() }
                            GlassButton { text: qsTr("Bloquear"); width: 180; height: 38; onClicked: system.lock() }
                        }

                        Text { text: qsTr("Perfiles de energía"); color: Theme.textMuted; font.pixelSize: 12; font.family: Theme.fontFamily; font.weight: Font.DemiBold }

                        Text { text: qsTr("Perfil actual: ") + settings.currentPowerProfile(); color: Theme.textSecondary; font.pixelSize: 13; font.family: Theme.fontFamily }

                        ListView {
                            width: parent.width
                            height: 110
                            clip: true
                            spacing: 6
                            model: settings.powerProfiles()
                            delegate: GlassButton { text: modelData; width: 220; height: 34; onClicked: settings.setPowerProfile(modelData) }
                        }
                    }
                }

                // ==========================================================
                // 6 FECHA Y HORA
                // ==========================================================
                ScrollView {
                    clip: true
                    Column {
                        width: parent.width
                        spacing: 16

                        Text { text: settings.currentDateTime(); color: Theme.textPrimary; font.pixelSize: 20; font.family: Theme.fontFamily }
                        Text { text: qsTr("Hora automática (NTP)"); color: Theme.textSecondary; font.pixelSize: 13; font.family: Theme.fontFamily }

                        Row {
                            spacing: 10
                            GlassButton { text: qsTr("Activar"); width: 120; height: 38; onClicked: settings.setAutoTime(true) }
                            GlassButton { text: qsTr("Desactivar"); width: 120; height: 38; onClicked: settings.setAutoTime(false) }
                        }

                        Text { text: qsTr("Zona horaria"); color: Theme.textSecondary; font.pixelSize: 13; font.family: Theme.fontFamily }

                        Row {
                            spacing: 10
                            TextField {
                                id: tzField
                                width: 320
                                height: 38
                                placeholderText: qsTr("Ej.: America/Santiago")
                                color: Theme.textPrimary
                                font.pixelSize: 13
                                font.family: Theme.fontFamily
                                background: Rectangle { radius: 10; color: "#1affffff"; border.width: 1; border.color: "#33ffffff" }
                            }
                            GlassButton { text: qsTr("Aplicar"); width: 110; height: 38; onClicked: settings.setTimeZone(tzField.text) }
                        }
                    }
                }

                // ==========================================================
                // 7 TECLADO
                // ==========================================================
                ScrollView {
                    clip: true
                    Column {
                        width: parent.width
                        spacing: 16

                        Text { text: qsTr("Distribución"); color: Theme.textSecondary; font.pixelSize: 13; font.family: Theme.fontFamily }

                        Row {
                            spacing: 10
                            TextField {
                                id: kbField
                                width: 200
                                height: 38
                                placeholderText: qsTr("Ej.: us, es, latam")
                                color: Theme.textPrimary
                                font.pixelSize: 13
                                font.family: Theme.fontFamily
                                background: Rectangle { radius: 10; color: "#1affffff"; border.width: 1; border.color: "#33ffffff" }
                            }
                            GlassButton { text: qsTr("Aplicar"); width: 110; height: 38; onClicked: settings.setKeyboardLayout(kbField.text) }
                        }

                        Text { text: qsTr("Repetición de teclas"); color: Theme.textMuted; font.pixelSize: 12; font.family: Theme.fontFamily; font.weight: Font.DemiBold }

                        Row {
                            spacing: 14
                            Text { text: qsTr("Retardo"); color: Theme.textPrimary; font.pixelSize: 14; font.family: Theme.fontFamily; width: 120; anchors.verticalCenter: parent.verticalCenter }
                            Slider { width: 300; from: 100; to: 1000; value: settings.repeatDelay; onMoved: settings.repeatDelay = Math.round(value) }
                            Text { text: settings.repeatDelay + " ms"; color: Theme.textSecondary; font.pixelSize: 13; font.family: Theme.fontFamily; anchors.verticalCenter: parent.verticalCenter }
                        }

                        Row {
                            spacing: 14
                            Text { text: qsTr("Velocidad"); color: Theme.textPrimary; font.pixelSize: 14; font.family: Theme.fontFamily; width: 120; anchors.verticalCenter: parent.verticalCenter }
                            Slider { width: 300; from: 10; to: 100; value: settings.repeatRate; onMoved: settings.repeatRate = Math.round(value) }
                            Text { text: settings.repeatRate + " /s"; color: Theme.textSecondary; font.pixelSize: 13; font.family: Theme.fontFamily; anchors.verticalCenter: parent.verticalCenter }
                        }

                        Text { text: qsTr("Fuente de la interfaz"); color: Theme.textMuted; font.pixelSize: 12; font.family: Theme.fontFamily; font.weight: Font.DemiBold }

                        ComboBox {
                            width: 340
                            model: settings.listFonts()
                            onActivated: settings.setFont(currentText)
                        }

                        Text { text: qsTr("Atajos multimedia registrados"); color: Theme.textMuted; font.pixelSize: 12; font.family: Theme.fontFamily; font.weight: Font.DemiBold }

                        ListView {
                            width: parent.width
                            height: 120
                            clip: true
                            spacing: 2
                            model: settings.listKeyboardShortcuts()
                            delegate: Rectangle {
                                width: ListView.view.width
                                height: 24
                                radius: 6
                                color: "#0fffffff"
                                Text { x: 10; anchors.verticalCenter: parent.verticalCenter; text: modelData; color: Theme.textPrimary; font.pixelSize: 11; font.family: Theme.fontFamily }
                            }
                        }

                        GlassButton { text: qsTr("Abrir ajustes de teclado del sistema"); width: 300; height: 38; onClicked: settings.openSystemKeyboardSettings() }
                    }
                }

                // ==========================================================
                // 8 RATÓN
                // ==========================================================
                ScrollView {
                    clip: true
                    Column {
                        width: parent.width
                        spacing: 16

                        Row {
                            spacing: 14
                            Text { text: qsTr("Velocidad del puntero"); color: Theme.textPrimary; font.pixelSize: 14; font.family: Theme.fontFamily; width: 200; anchors.verticalCenter: parent.verticalCenter }
                            Slider { width: 300; from: -1.0; to: 1.0; value: settings.mouseSpeed; onMoved: settings.setMouseSpeed(value) }
                            Text { text: settings.mouseSpeed.toFixed(2); color: Theme.textSecondary; font.pixelSize: 13; font.family: Theme.fontFamily; anchors.verticalCenter: parent.verticalCenter }
                        }

                        Row {
                            spacing: 14
                            Text { text: qsTr("Desplazamiento natural"); color: Theme.textPrimary; font.pixelSize: 14; font.family: Theme.fontFamily; width: 220; anchors.verticalCenter: parent.verticalCenter }
                            Switch { checked: settings.mouseNaturalScroll; onToggled: settings.mouseNaturalScroll = checked }
                        }

                        Text { text: qsTr("Se aplica con gsettings (org.gnome.desktop.peripherals.mouse)."); color: Theme.textMuted; font.pixelSize: 11; font.family: Theme.fontFamily }
                    }
                }

                // ==========================================================
                // 9 TOUCHPAD
                // ==========================================================
                ScrollView {
                    clip: true
                    Column {
                        width: parent.width
                        spacing: 16

                        Row {
                            spacing: 14
                            Text { text: qsTr("Touchpad activado"); color: Theme.textPrimary; font.pixelSize: 14; font.family: Theme.fontFamily; width: 220; anchors.verticalCenter: parent.verticalCenter }
                            Switch { checked: settings.touchpadEnabled; onToggled: settings.touchpadEnabled = checked }
                        }

                        Row {
                            spacing: 14
                            Text { text: qsTr("Toque para hacer clic"); color: Theme.textPrimary; font.pixelSize: 14; font.family: Theme.fontFamily; width: 220; anchors.verticalCenter: parent.verticalCenter }
                            Switch { checked: settings.touchpadTapToClick; onToggled: settings.touchpadTapToClick = checked }
                        }

                        Row {
                            spacing: 14
                            Text { text: qsTr("Desplazamiento natural"); color: Theme.textPrimary; font.pixelSize: 14; font.family: Theme.fontFamily; width: 220; anchors.verticalCenter: parent.verticalCenter }
                            Switch { checked: settings.touchpadNaturalScroll; onToggled: settings.touchpadNaturalScroll = checked }
                        }

                        Text { text: qsTr("Se aplica con gsettings (org.gnome.desktop.peripherals.touchpad)."); color: Theme.textMuted; font.pixelSize: 11; font.family: Theme.fontFamily }
                    }
                }

                // ==========================================================
                // 10 APARIENCIA
                // ==========================================================
                ScrollView {
                    clip: true
                    Column {
                        width: parent.width
                        spacing: 16

                        Text { text: qsTr("Iconos"); color: Theme.textMuted; font.pixelSize: 12; font.family: Theme.fontFamily; font.weight: Font.DemiBold }
                        ComboBox {
                            id: iconThemeBox
                            width: 340
                            model: settings.listIconThemes()
                            Component.onCompleted: currentIndex = Math.max(0, find(settings.iconTheme))
                            onActivated: settings.iconTheme = currentText
                        }

                        Text { text: qsTr("Cursor"); color: Theme.textMuted; font.pixelSize: 12; font.family: Theme.fontFamily; font.weight: Font.DemiBold }
                        ComboBox {
                            id: cursorThemeBox
                            width: 340
                            model: settings.listCursorThemes()
                            Component.onCompleted: currentIndex = Math.max(0, find(settings.cursorTheme))
                            onActivated: settings.cursorTheme = currentText
                        }

                        Row {
                            spacing: 14
                            Text { text: qsTr("Tamaño del cursor"); color: Theme.textPrimary; font.pixelSize: 14; font.family: Theme.fontFamily; width: 200; anchors.verticalCenter: parent.verticalCenter }
                            Slider { width: 300; from: 16; to: 64; value: settings.cursorSize; onMoved: settings.cursorSize = Math.round(value) }
                            Text { text: settings.cursorSize + " px"; color: Theme.textSecondary; font.pixelSize: 13; font.family: Theme.fontFamily; anchors.verticalCenter: parent.verticalCenter }
                        }

                        Text { text: qsTr("Esquema de color"); color: Theme.textMuted; font.pixelSize: 12; font.family: Theme.fontFamily; font.weight: Font.DemiBold }
                        ComboBox {
                            width: 340
                            model: [qsTr("Claro"), qsTr("Oscuro"), qsTr("Predeterminado")]
                            Component.onCompleted: currentIndex =
                                settings.colorScheme === "prefer-light" ? 0
                                : settings.colorScheme === "prefer-dark" ? 1 : 2
                            onActivated: settings.colorScheme = index === 0 ? "prefer-light" : index === 1 ? "prefer-dark" : "default"
                        }

                        Rectangle { width: parent.width; height: 1; color: "#33ffffff" }

                        Text { text: qsTr("Iconos y accesos directos del escritorio"); color: Theme.textMuted; font.pixelSize: 12; font.family: Theme.fontFamily; font.weight: Font.DemiBold }
                        Text {
                            text: qsTr("Muestra estos accesos directamente sobre el fondo de pantalla. También puedes anclar aplicaciones al escritorio desde el menú Inicio.")
                            color: Theme.textSecondary
                            font.pixelSize: 12
                            font.family: Theme.fontFamily
                            wrapMode: Text.WordWrap
                            width: parent.width
                        }

                        Row {
                            spacing: 14
                            Text { text: qsTr("Equipo"); color: Theme.textPrimary; font.pixelSize: 14; font.family: Theme.fontFamily; width: 220; anchors.verticalCenter: parent.verticalCenter }
                            Switch { checked: settings.desktopShowComputer; onToggled: settings.desktopShowComputer = checked }
                        }

                        Row {
                            spacing: 14
                            Text { text: qsTr("Usuario"); color: Theme.textPrimary; font.pixelSize: 14; font.family: Theme.fontFamily; width: 220; anchors.verticalCenter: parent.verticalCenter }
                            Switch { checked: settings.desktopShowUser; onToggled: settings.desktopShowUser = checked }
                        }

                        Row {
                            spacing: 14
                            Text { text: qsTr("Papelera"); color: Theme.textPrimary; font.pixelSize: 14; font.family: Theme.fontFamily; width: 220; anchors.verticalCenter: parent.verticalCenter }
                            Switch { checked: settings.desktopShowTrash; onToggled: settings.desktopShowTrash = checked }
                        }

                        Rectangle { width: parent.width; height: 1; color: "#33ffffff" }

                        Text { text: qsTr("Notificaciones"); color: Theme.textMuted; font.pixelSize: 12; font.family: Theme.fontFamily; font.weight: Font.DemiBold }
                        Text {
                            text: qsTr("Estas opciones controlan el aviso emergente y el icono de la bandeja. El widget Estado del sistema sigue mostrando todas las notificaciones aunque se silencien.")
                            color: Theme.textSecondary
                            font.pixelSize: 12
                            font.family: Theme.fontFamily
                            wrapMode: Text.WordWrap
                            width: parent.width
                        }

                        Row {
                            spacing: 14
                            Text { text: qsTr("Silenciar notificaciones"); color: Theme.textPrimary; font.pixelSize: 14; font.family: Theme.fontFamily; width: 220; anchors.verticalCenter: parent.verticalCenter }
                            Switch { checked: settings.notificationsMuted; onToggled: settings.notificationsMuted = checked }
                        }

                        Row {
                            spacing: 14
                            Text { text: qsTr("Mostrar icono en la bandeja"); color: Theme.textPrimary; font.pixelSize: 14; font.family: Theme.fontFamily; width: 220; anchors.verticalCenter: parent.verticalCenter }
                            Switch { checked: settings.notificationsTrayIcon; onToggled: settings.notificationsTrayIcon = checked }
                        }

                        Row {
                            spacing: 14
                            Text { text: qsTr("Mostrar notificación emergente"); color: Theme.textPrimary; font.pixelSize: 14; font.family: Theme.fontFamily; width: 220; anchors.verticalCenter: parent.verticalCenter }
                            Switch { checked: settings.notificationsPopup; onToggled: settings.notificationsPopup = checked }
                        }
                    }
                }

                // ==========================================================
                // 11 PANTALLA
                // ==========================================================
                ScrollView {
                    clip: true
                    Column {
                        width: parent.width
                        spacing: 16

                        Text { text: qsTr("Resolución"); color: Theme.textMuted; font.pixelSize: 12; font.family: Theme.fontFamily; font.weight: Font.DemiBold }

                        ListView {
                            width: parent.width
                            height: 90
                            clip: true
                            spacing: 2
                            model: settings.listDisplayOutputs()
                            delegate: Rectangle {
                                width: ListView.view.width
                                height: 26
                                radius: 6
                                color: "#0fffffff"
                                Text { x: 10; anchors.verticalCenter: parent.verticalCenter; text: modelData; color: Theme.textPrimary; font.pixelSize: 12; font.family: Theme.fontFamily }
                            }
                            Text {
                                anchors.centerIn: parent
                                visible: parent.count === 0
                                text: qsTr("No se detectaron salidas (xrandr)")
                                color: Theme.textMuted
                                font.pixelSize: 12
                                font.family: Theme.fontFamily
                            }
                        }

                        Row {
                            spacing: 10
                            TextField {
                                id: outputField
                                width: 180
                                height: 38
                                placeholderText: qsTr("Salida (HDMI-1)")
                                color: Theme.textPrimary
                                font.pixelSize: 13
                                font.family: Theme.fontFamily
                                background: Rectangle { radius: 10; color: "#1affffff"; border.width: 1; border.color: "#33ffffff" }
                            }
                            TextField {
                                id: modeField
                                width: 180
                                height: 38
                                placeholderText: qsTr("Modo (1920x1080)")
                                color: Theme.textPrimary
                                font.pixelSize: 13
                                font.family: Theme.fontFamily
                                background: Rectangle { radius: 10; color: "#1affffff"; border.width: 1; border.color: "#33ffffff" }
                            }
                            GlassButton { text: qsTr("Aplicar"); width: 110; height: 38; onClicked: settings.setDisplayMode(outputField.text, modeField.text) }
                        }

                        Text { text: qsTr("Brillo"); color: Theme.textMuted; font.pixelSize: 12; font.family: Theme.fontFamily; font.weight: Font.DemiBold }

                        Row {
                            spacing: 14
                            Slider { width: 420; from: 0; to: 100; value: settings.brightness >= 0 ? settings.brightness : 50; onMoved: settings.setBrightness(Math.round(value)) }
                            Text { text: settings.brightness >= 0 ? settings.brightness + "%" : qsTr("N/D"); color: Theme.textSecondary; font.pixelSize: 13; font.family: Theme.fontFamily; anchors.verticalCenter: parent.verticalCenter }
                        }

                        Text { text: qsTr("Fondo de pantalla"); color: Theme.textMuted; font.pixelSize: 12; font.family: Theme.fontFamily; font.weight: Font.DemiBold }

                        Row {
                            width: parent.width
                            spacing: 10
                            TextField {
                                id: wpField
                                width: parent.width - 130
                                height: 38
                                text: settings.wallpaper
                                placeholderText: qsTr("Ruta o file:// de la imagen")
                                color: Theme.textPrimary
                                font.pixelSize: 13
                                font.family: Theme.fontFamily
                                background: Rectangle { radius: 10; color: "#1affffff"; border.width: 1; border.color: "#33ffffff" }
                            }
                            GlassButton { text: qsTr("Guardar"); width: 110; height: 38; onClicked: settings.wallpaper = wpField.text }
                        }

                        Text { text: qsTr("Se aplica al reiniciar stellar-background (STELLAR_WALLPAPER)."); color: Theme.textMuted; font.pixelSize: 11; font.family: Theme.fontFamily; wrapMode: Text.WordWrap; width: parent.width }
                    }
                }

            // ==========================================================
            // 12 SISTEMA
            // ==========================================================
            ScrollView {
                clip: true
                Column {
                    width: parent.width
                    spacing: 16

                    Text { text: qsTr("Información del sistema"); color: Theme.textMuted; font.pixelSize: 12; font.family: Theme.fontFamily; font.weight: Font.DemiBold }

                    Rectangle {
                        width: parent.width
                        height: 150
                        radius: 12
                        color: "#14ffffff"
                        border.width: 1
                        border.color: "#22ffffff"

                        Column {
                            anchors.fill: parent
                            anchors.margins: 14
                            spacing: 8

                            Text { text: qsTr("Equipo: ") + settings.hostName(); color: Theme.textPrimary; font.pixelSize: 14; font.family: Theme.fontFamily }
                            Text { text: qsTr("Kernel: ") + settings.kernelVersion(); color: Theme.textSecondary; font.pixelSize: 13; font.family: Theme.fontFamily }
                            Text { text: qsTr("Sesión: ") + settings.sessionType() + " · " + settings.desktopName(); color: Theme.textSecondary; font.pixelSize: 13; font.family: Theme.fontFamily }
                            Text { text: qsTr("Encendido: ") + settings.uptime(); color: Theme.textSecondary; font.pixelSize: 13; font.family: Theme.fontFamily }
                        }
                    }

                    Text { text: qsTr("Acciones"); color: Theme.textMuted; font.pixelSize: 12; font.family: Theme.fontFamily; font.weight: Font.DemiBold }

                    Row {
                        spacing: 10
                        GlassButton { text: qsTr("Abrir terminal"); width: 190; height: 38; onClicked: settings.openTerminal() }
                        GlassButton { text: qsTr("Reiniciar panel"); width: 190; height: 38; onClicked: settings.restartPanel() }
                    }

                    Text { text: qsTr("El Centro de configuración centraliza panel, menú, widgets, red, sonido, energía, fecha, teclado, ratón, touchpad, apariencia, pantalla y sistema."); color: Theme.textMuted; font.pixelSize: 11; font.family: Theme.fontFamily; wrapMode: Text.WordWrap; width: parent.width }
                }
            }

            // ==========================================================
            // 13 DISPOSITIVOS
            // ==========================================================
            ScrollView {
                clip: true
                Column {
                    width: parent.width
                    spacing: 16

                    Component.onCompleted: devices.refresh()

                    Text { text: qsTr("Conecta y administra impresoras, Bluetooth, cámaras y discos."); color: Theme.textSecondary; font.pixelSize: 13; font.family: Theme.fontFamily; wrapMode: Text.WordWrap; width: parent.width }

                    Row {
                        spacing: 10
                        GlassButton { text: qsTr("Actualizar todo"); width: 150; height: 36; onClicked: devices.refresh() }
                        GlassButton { text: qsTr("Ajustes de impresoras"); width: 200; height: 36; onClicked: devices.openPrinterSettings() }
                        GlassButton { text: qsTr("Utilidad de discos"); width: 180; height: 36; onClicked: devices.openDiskUtility() }
                    }

                    Rectangle { width: parent.width; height: 1; color: "#33ffffff" }

                    Text { text: qsTr("Impresoras"); color: Theme.textMuted; font.pixelSize: 12; font.family: Theme.fontFamily; font.weight: Font.DemiBold }

                    Repeater {
                        model: devices.printers
                        delegate: Rectangle {
                            width: parent.width
                            height: 48
                            radius: 9
                            color: "#12ffffff"
                            border.width: 1
                            border.color: "#1fffffff"

                            Text {
                                x: 12
                                anchors.verticalCenter: parent.verticalCenter
                                width: parent.width - 40
                                text: (modelData.isDefault ? "★ " : "") + modelData.name
                                color: Theme.textPrimary
                                font.pixelSize: 13
                                font.family: Theme.fontFamily
                                font.weight: Font.DemiBold
                                elide: Text.ElideRight
                            }

                            Text {
                                x: 12
                                y: 27
                                width: parent.width - 30
                                text: modelData.status
                                color: Theme.textSecondary
                                font.pixelSize: 10
                                font.family: Theme.fontFamily
                                elide: Text.ElideRight
                            }
                        }
                    }

                    Text {
                        visible: devices.printers.length === 0
                        text: qsTr("No se detectaron impresoras (CUPS no disponible o sin colas).")
                        color: Theme.textMuted
                        font.pixelSize: 11
                        font.family: Theme.fontFamily
                        wrapMode: Text.WordWrap
                        width: parent.width
                    }

                    Rectangle { width: parent.width; height: 1; color: "#33ffffff" }

                    Text { text: qsTr("Bluetooth"); color: Theme.textMuted; font.pixelSize: 12; font.family: Theme.fontFamily; font.weight: Font.DemiBold }

                    Row {
                        spacing: 14
                        Text { text: qsTr("Bluetooth activado"); color: Theme.textPrimary; font.pixelSize: 14; font.family: Theme.fontFamily; anchors.verticalCenter: parent.verticalCenter; width: 220 }
                        Switch {
                            checked: devices.bluetoothEnabled
                            enabled: devices.bluetoothAvailable
                            onToggled: devices.setBluetoothEnabled(checked)
                        }
                        GlassButton {
                            text: devices.bluetoothScanning ? qsTr("Buscando…") : qsTr("Buscar dispositivos")
                            width: 190
                            height: 36
                            enabled: devices.bluetoothAvailable && !devices.bluetoothScanning
                            onClicked: devices.scanBluetooth()
                        }
                        GlassButton {
                            text: qsTr("Ajustes de Bluetooth")
                            width: 180
                            height: 36
                            onClicked: devices.openBluetoothSettings()
                        }
                    }

                    Text {
                        visible: !devices.bluetoothAvailable
                        text: qsTr("No se detectó un adaptador Bluetooth (BlueZ/bluetoothctl).")
                        color: Theme.textMuted
                        font.pixelSize: 11
                        font.family: Theme.fontFamily
                        wrapMode: Text.WordWrap
                        width: parent.width
                    }

                    Repeater {
                        model: devices.bluetoothDevices
                        delegate: Rectangle {
                            width: parent.width
                            height: 48
                            radius: 9
                            color: "#12ffffff"
                            border.width: 1
                            border.color: "#1fffffff"

                            Column {
                                x: 12
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 1
                                Text {
                                    text: modelData.name
                                    color: Theme.textPrimary
                                    font.pixelSize: 12
                                    font.family: Theme.fontFamily
                                    font.weight: Font.DemiBold
                                }
                                Text {
                                    text: modelData.address + (modelData.connected ? qsTr(" · Conectado") : "")
                                    color: modelData.connected ? "#9befb3" : Theme.textMuted
                                    font.pixelSize: 10
                                    font.family: Theme.fontFamily
                                }
                            }

                            GlassButton {
                                anchors.right: parent.right
                                anchors.rightMargin: 8
                                anchors.verticalCenter: parent.verticalCenter
                                width: 120
                                height: 32
                                text: modelData.connected ? qsTr("Desconectar") : qsTr("Conectar")
                                onClicked: modelData.connected
                                    ? devices.disconnectBluetooth(modelData.address)
                                    : devices.connectBluetooth(modelData.address)
                            }
                        }
                    }

                    Rectangle { width: parent.width; height: 1; color: "#33ffffff" }

                    Text { text: qsTr("Cámaras"); color: Theme.textMuted; font.pixelSize: 12; font.family: Theme.fontFamily; font.weight: Font.DemiBold }

                    Repeater {
                        model: devices.cameras
                        delegate: Rectangle {
                            width: parent.width
                            height: 46
                            radius: 9
                            color: "#12ffffff"
                            border.width: 1
                            border.color: "#1fffffff"

                            Text {
                                x: 12
                                anchors.verticalCenter: parent.verticalCenter
                                width: parent.width - 140
                                text: modelData.name + "  ·  " + modelData.device
                                color: Theme.textPrimary
                                font.pixelSize: 12
                                font.family: Theme.fontFamily
                                elide: Text.ElideRight
                            }

                            GlassButton {
                                anchors.right: parent.right
                                anchors.rightMargin: 8
                                anchors.verticalCenter: parent.verticalCenter
                                width: 110
                                height: 32
                                text: qsTr("Abrir cámara")
                                onClicked: devices.openCamera(modelData.device)
                            }
                        }
                    }

                    Text {
                        visible: devices.cameras.length === 0
                        text: qsTr("No se detectaron cámaras V4L2.")
                        color: Theme.textMuted
                        font.pixelSize: 11
                        font.family: Theme.fontFamily
                    }

                    Rectangle { width: parent.width; height: 1; color: "#33ffffff" }

                    Text { text: qsTr("Discos y unidades"); color: Theme.textMuted; font.pixelSize: 12; font.family: Theme.fontFamily; font.weight: Font.DemiBold }

                    Repeater {
                        model: devices.disks
                        delegate: Rectangle {
                            width: parent.width
                            height: 54
                            radius: 9
                            color: "#12ffffff"
                            border.width: 1
                            border.color: "#1fffffff"

                            property string mountPoint: modelData.mountpoint
                                ? modelData.mountpoint : ""
                            property bool protectedDisk: mountPoint === "/"
                                || mountPoint.indexOf("/boot") === 0
                                || mountPoint.indexOf("/usr") === 0
                                || mountPoint.indexOf("/var") === 0
                                || mountPoint === "[SWAP]"
                                || modelData.fstype === "swap"

                            Column {
                                x: 12
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 1
                                Text {
                                    text: (modelData.label && modelData.label.length > 0
                                        ? modelData.label + " · " : "")
                                        + modelData.name + "  (" + modelData.size + ")"
                                    color: Theme.textPrimary
                                    font.pixelSize: 12
                                    font.family: Theme.fontFamily
                                    font.weight: Font.DemiBold
                                }
                                Text {
                                    text: (modelData.model && modelData.model.length > 0
                                        ? modelData.model + " · " : "")
                                        + (modelData.fstype && modelData.fstype.length > 0
                                            ? modelData.fstype : qsTr("sin formato"))
                                        + " · "
                                        + (parent.parent.mountPoint.length > 0
                                            ? parent.parent.mountPoint : qsTr("sin montar"))
                                    color: Theme.textMuted
                                    font.pixelSize: 10
                                    font.family: Theme.fontFamily
                                    elide: Text.ElideRight
                                }
                            }

                            Text {
                                visible: parent.protectedDisk
                                anchors.right: parent.right
                                anchors.rightMargin: 12
                                anchors.verticalCenter: parent.verticalCenter
                                text: qsTr("Sistema")
                                color: Theme.textMuted
                                font.pixelSize: 11
                                font.family: Theme.fontFamily
                            }

                            Row {
                                visible: !parent.protectedDisk
                                anchors.right: parent.right
                                anchors.rightMargin: 8
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 6

                                GlassButton {
                                    width: 100
                                    height: 32
                                    text: parent.parent.mountPoint.length > 0
                                        ? qsTr("Desmontar") : qsTr("Montar")
                                    onClicked: parent.parent.mountPoint.length > 0
                                        ? devices.unmountDisk(modelData.name)
                                        : devices.mountDisk(modelData.name)
                                }

                                GlassButton {
                                    visible: modelData.removable
                                    width: 90
                                    height: 32
                                    text: qsTr("Expulsar")
                                    onClicked: devices.ejectDisk(modelData.name)
                                }
                            }
                        }
                    }

                    Text {
                        visible: devices.disks.length === 0
                        text: qsTr("No se detectaron discos (lsblk no disponible).")
                        color: Theme.textMuted
                        font.pixelSize: 11
                        font.family: Theme.fontFamily
                    }
                }
            }

            // ==========================================================
            // 14 APLICACIONES PREDETERMINADAS
            // ==========================================================
            ScrollView {
                clip: true
                Column {
                    width: parent.width
                    spacing: 14

                    Component.onCompleted: defaultApps.refresh()

                    Text {
                        text: qsTr("Elige qué aplicación abre cada tipo de contenido. Los cambios se guardan en las asociaciones XDG del usuario.")
                        color: Theme.textSecondary
                        font.pixelSize: 13
                        font.family: Theme.fontFamily
                        wrapMode: Text.WordWrap
                        width: parent.width
                    }

                    Row {
                        spacing: 10
                        GlassButton { text: qsTr("Actualizar"); width: 130; height: 36; onClicked: defaultApps.refresh() }
                        GlassButton { text: qsTr("Ajustes del sistema"); width: 190; height: 36; onClicked: defaultApps.openSystemSettings() }
                    }

                    Rectangle { width: parent.width; height: 1; color: "#33ffffff" }

                    Repeater {
                        model: defaultApps.categories
                        delegate: Rectangle {
                            id: categoryRow
                            width: parent.width
                            height: 72
                            radius: 10
                            color: "#12ffffff"
                            border.width: 1
                            border.color: "#1fffffff"

                            property var category: modelData

                            function currentChoiceIndex() {
                                const choices = category.choices
                                for (let i = 0; i < choices.length; ++i) {
                                    if (choices[i].file === category.current) {
                                        return i
                                    }
                                }
                                return -1
                            }

                            Text {
                                x: 12
                                y: 8
                                text: category.label
                                color: Theme.textPrimary
                                font.pixelSize: 13
                                font.family: Theme.fontFamily
                                font.weight: Font.DemiBold
                            }

                            StellarIcon {
                                x: 12
                                y: 34
                                size: 22
                                showTile: false
                                iconName: category.currentIcon
                            }

                            Text {
                                x: 42
                                y: 37
                                width: 180
                                text: category.currentName
                                color: Theme.textSecondary
                                font.pixelSize: 11
                                font.family: Theme.fontFamily
                                elide: Text.ElideRight
                            }

                            ComboBox {
                                id: categoryCombo
                                x: 240
                                y: 28
                                width: 360
                                height: 34
                                model: category.choices
                                textRole: "name"
                                currentIndex: categoryRow.currentChoiceIndex()

                                background: Rectangle {
                                    radius: 9
                                    color: "#1affffff"
                                    border.width: 1
                                    border.color: "#33ffffff"
                                }

                                contentItem: Text {
                                    leftPadding: 12
                                    rightPadding: 24
                                    text: categoryCombo.displayText
                                    color: Theme.textPrimary
                                    font.family: Theme.fontFamily
                                    font.pixelSize: 12
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight
                                }

                                onActivated: defaultApps.setDefault(category.id,
                                                                    category.choices[index].file)
                            }
                        }
                    }
                }
            }
            }
        }
    }
}
