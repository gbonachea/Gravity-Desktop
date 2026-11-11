import QtQuick
import "../common"
import "../common/Theme.js" as Theme

/*!
 * Native network + volume panel. It is a thin QML view over SystemManager:
 * everything it does (scan, toggle Wi-Fi, set volume, mute) is a Q_INVOKABLE
 * DBus/native call implemented in C++.
 */
Item {
    id: root

    property bool opened: false
    property int dragVolume: -1

    visible: opacity > 0.01
    opacity: opened ? 1.0 : 0.0
    z: 90

    Behavior on opacity { NumberAnimation { duration: Theme.fast } }

    function open() {
        opened = true
        system.scanWifi()
    }

    function close() {
        opened = false
        dragVolume = -1
    }

    function volumeAt(x) {
        if (volumeBar.width <= 0)
            return system.volume
        return Math.round(Theme.clamp(x / volumeBar.width, 0, 1) * 100)
    }

    Rectangle {
        anchors.fill: parent
        color: "#73040f22"

        MouseArea {
            anchors.fill: parent
            onClicked: root.close()
        }
    }

    GlassCard {
        id: panel
        width: 440
        height: 430
        anchors.centerIn: parent
        radius: Theme.radius
        tintTop: Theme.menuTop
        tintBottom: Theme.menuBottom
        borderColor: Theme.menuBorder
        shadowOffset: 10

        // Swallow clicks so they do not reach the dimmed backdrop.
        MouseArea { anchors.fill: parent }

        Text {
            x: 20
            y: 16
            text: qsTr("Red y sonido")
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: 17
            font.weight: Font.DemiBold
        }

        Text {
            x: 20
            y: 42
            width: 290
            text: system.networkManagerAvailable
                ? qsTr("Conectividad: %1").arg(system.connectivity)
                : qsTr("NetworkManager no está disponible")
            color: Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: 10
            elide: Text.ElideRight
        }

        GlassButton {
            x: panel.width - width - 20
            y: 14
            width: 76
            height: 30
            text: qsTr("Cerrar")
            onClicked: root.close()
        }

        // ------------------------------------------------------------ Wi-Fi
        Rectangle {
            x: 20
            y: 66
            width: panel.width - 40
            height: 52
            radius: 10
            color: "#14ffffff"
            border.width: 1
            border.color: "#26ffffff"

            Column {
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2

                Text {
                    text: qsTr("Wi-Fi")
                    color: Theme.textPrimary
                    font.family: Theme.fontFamily
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                }

                Text {
                    text: system.wifiEnabled ? qsTr("Activado") : qsTr("Desactivado")
                    color: system.wifiEnabled ? "#9befb3" : Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: 9
                }
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
                Behavior on color { ColorAnimation { duration: Theme.fast } }

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

        // ---------------------------------------------------------- Volumen
        Text {
            x: 20
            y: 132
            text: qsTr("VOLUMEN")
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: 9
            font.letterSpacing: 1.0
            font.weight: Font.DemiBold
        }

        Text {
            anchors.right: parent.right
            anchors.rightMargin: 20
            y: 130
            text: system.muted ? qsTr("Silenciado") : system.volume + "%"
            color: system.muted ? "#ffb3bd" : Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: 10
            font.weight: Font.DemiBold
        }

        Item {
            id: volumeBar
            x: 20
            y: 152
            width: panel.width - 40
            height: 10

            Rectangle {
                anchors.fill: parent
                radius: 5
                color: "#26ffffff"
            }

            Rectangle {
                width: parent.width
                    * Math.max(0, root.dragVolume >= 0 ? root.dragVolume : system.volume) / 100
                height: parent.height
                radius: 5
                opacity: system.muted ? 0.45 : 1.0
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: "#59aaff" }
                    GradientStop { position: 1.0; color: "#2a7cf0" }
                }
            }

            MouseArea {
                anchors.fill: parent
                anchors.topMargin: -6
                anchors.bottomMargin: -6
                cursorShape: Qt.PointingHandCursor

                onPressed: function(mouse) {
                    root.dragVolume = root.volumeAt(mouse.x)
                }
                onPositionChanged: function(mouse) {
                    if (pressed)
                        root.dragVolume = root.volumeAt(mouse.x)
                }
                onReleased: function(mouse) {
                    system.setVolume(root.volumeAt(mouse.x))
                    root.dragVolume = -1
                }
                onCanceled: root.dragVolume = -1
            }
        }

        Row {
            x: 20
            y: 182
            spacing: 6

            GlassButton {
                width: 138
                height: 32
                text: system.muted ? qsTr("Activar sonido") : qsTr("Silenciar")
                onClicked: system.toggleMute()
            }

            GlassButton {
                width: 58
                height: 32
                text: "−"
                onClicked: system.decreaseVolume()
            }

            GlassButton {
                width: 58
                height: 32
                text: "+"
                onClicked: system.increaseVolume()
            }
        }

        // ------------------------------------------------------- Interfaces
        Text {
            x: 20
            y: 228
            text: qsTr("INTERFACES DE RED")
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: 9
            font.letterSpacing: 1.0
            font.weight: Font.DemiBold
        }

        ListView {
            x: 20
            y: 248
            width: panel.width - 40
            height: 58
            clip: true
            spacing: 2
            model: system.interfaces

            delegate: Rectangle {
                width: ListView.view.width
                height: 26
                radius: 8
                color: "#0fffffff"

                Text {
                    x: 8
                    width: 130
                    anchors.verticalCenter: parent.verticalCenter
                    text: modelData.name
                    color: Theme.textPrimary
                    font.family: Theme.fontFamily
                    font.pixelSize: 10
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }

                Text {
                    x: 144
                    width: 104
                    anchors.verticalCenter: parent.verticalCenter
                    text: modelData.type
                    color: Theme.textSecondary
                    font.family: Theme.fontFamily
                    font.pixelSize: 9
                    elide: Text.ElideRight
                }

                Text {
                    anchors.right: parent.right
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    text: modelData.state
                    color: modelData.connected ? "#9befb3" : Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: 9
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
                font.pixelSize: 10
            }
        }

        // ------------------------------------------------------ Redes Wi-Fi
        Text {
            x: 20
            y: 318
            text: qsTr("REDES WI-FI")
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: 9
            font.letterSpacing: 1.0
            font.weight: Font.DemiBold
        }

        GlassButton {
            x: panel.width - width - 20
            y: 310
            width: 112
            height: 28
            text: system.scanning ? qsTr("Escaneando…") : qsTr("Escanear")
            onClicked: system.scanWifi()
        }

        ListView {
            x: 20
            y: 342
            width: panel.width - 40
            height: 70
            clip: true
            spacing: 2
            model: system.wifiNetworks

            delegate: Rectangle {
                width: ListView.view.width
                height: 32
                radius: 8
                color: "#0fffffff"

                Column {
                    anchors.left: parent.left
                    anchors.leftMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 1

                    Text {
                        width: 230
                        text: modelData.ssid
                        color: Theme.textPrimary
                        font.family: Theme.fontFamily
                        font.pixelSize: 10
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }

                    Text {
                        text: modelData.secured
                            ? modelData.security + " · " + modelData.band
                            : qsTr("Abierta") + " · " + modelData.band
                        color: Theme.textMuted
                        font.family: Theme.fontFamily
                        font.pixelSize: 8
                    }
                }

                Text {
                    anchors.right: parent.right
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    text: modelData.strength + "%"
                    color: Theme.textSecondary
                    font.family: Theme.fontFamily
                    font.pixelSize: 9
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
                font.pixelSize: 10
            }
        }
    }
}
