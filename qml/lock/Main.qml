import QtQuick
import "../common"
import "../common/Theme.js" as Theme

Item {
    id: root

    focus: true

    property string errorText: ""

    function submit() {
        if (lock.authenticating) {
            return
        }
        if (passwordInput.text.length === 0) {
            root.errorText = qsTr("Escribe tu contraseña")
            shake.restart()
            passwordInput.forceActiveFocus()
            return
        }
        root.errorText = ""
        lock.authenticate(passwordInput.text)
    }

    // Fondo: el mismo wallpaper del escritorio, oscurecido para que el
    // formulario destaque y no se pueda leer la sesión que queda detrás.
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#06142b" }
            GradientStop { position: 0.55; color: "#123a70" }
            GradientStop { position: 1.0; color: "#2c6cb0" }
        }
    }

    Image {
        anchors.fill: parent
        source: wallpaper.source
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        cache: true
        smooth: true
        visible: status === Image.Ready
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#d9081730" }
            GradientStop { position: 0.55; color: "#b9081730" }
            GradientStop { position: 1.0; color: "#e0081730" }
        }
    }

    // Reloj y fecha, como en la pantalla de bloqueo clásica.
    Column {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: Math.max(48, parent.height * 0.09)
        spacing: 4

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: clock.time
            color: "#ffffff"
            style: Text.Outline
            styleColor: "#8800142c"
            font.family: Theme.fontFamily
            font.pixelSize: 62
            font.weight: Font.Light
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: clock.dateText
            color: "#d7e8ff"
            style: Text.Outline
            styleColor: "#8800142c"
            font.family: Theme.fontFamily
            font.pixelSize: 16
            font.weight: Font.Medium
        }
    }

    // Tarjeta central de desbloqueo.
    GlassCard {
        id: card
        width: 420
        height: 348
        anchors.centerIn: parent
        anchors.verticalCenterOffset: 60
        radius: 22
        tintTop: Theme.menuTop
        tintBottom: Theme.menuBottom
        borderColor: Theme.menuBorder
        shadowOffset: 14

        opacity: 0.0
        scale: 0.96
        Component.onCompleted: {
            card.opacity = 1.0
            card.scale = 1.0
            passwordInput.forceActiveFocus()
        }
        Behavior on opacity { NumberAnimation { duration: Theme.slow } }
        Behavior on scale { NumberAnimation { duration: Theme.slow; easing.type: Easing.OutCubic } }

        // Candado dibujado con formas: no depende de emojis de la fuente.
        Item {
            id: padlock
            width: 58
            height: 64
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 24

            Rectangle {
                width: 30
                height: 30
                radius: 15
                anchors.horizontalCenter: parent.horizontalCenter
                color: "transparent"
                border.width: 5
                border.color: "#bfe3ff"
            }

            Rectangle {
                width: 48
                height: 36
                radius: 9
                y: 24
                anchors.horizontalCenter: parent.horizontalCenter
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#9fd0ff" }
                    GradientStop { position: 1.0; color: "#3f8fe0" }
                }
                border.width: 1
                border.color: "#d8efff"

                Rectangle {
                    width: 7
                    height: 14
                    radius: 3.5
                    y: 10
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: "#0b2a52"
                }
            }
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: padlock.bottom
            anchors.topMargin: 12
            text: lock.userName
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: 19
            font.weight: Font.DemiBold
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: padlock.bottom
            anchors.topMargin: 38
            text: qsTr("Equipo bloqueado")
            color: Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: 12
        }

        // Cuadro de contraseña con esquinas redondeadas.
        Rectangle {
            id: passwordBox
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: padlock.bottom
            anchors.topMargin: 68
            width: 320
            height: 50
            radius: 25
            color: passwordInput.activeFocus ? "#26ffffff" : "#16ffffff"
            border.width: 1
            border.color: passwordInput.activeFocus ? "#a7d6ff" : "#40ffffff"

            Behavior on color { ColorAnimation { duration: Theme.fast } }
            Behavior on border.color { ColorAnimation { duration: Theme.fast } }

            TextInput {
                id: passwordInput
                anchors.fill: parent
                anchors.leftMargin: 22
                anchors.rightMargin: 22
                verticalAlignment: TextInput.AlignVCenter
                echoMode: TextInput.Password
                passwordCharacter: "●"
                passwordMaskDelay: 0
                color: "#ffffff"
                selectionColor: "#3f8fe0"
                selectedTextColor: "#ffffff"
                font.family: Theme.fontFamily
                font.pixelSize: 15
                enabled: !lock.authenticating
                clip: true

                onAccepted: root.submit()
                onTextChanged: {
                    if (root.errorText.length > 0) {
                        root.errorText = ""
                    }
                }

                Text {
                    anchors.fill: parent
                    verticalAlignment: Text.AlignVCenter
                    visible: passwordInput.text.length === 0
                    text: qsTr("Contraseña")
                    color: "#8fb4d9"
                    font.family: Theme.fontFamily
                    font.pixelSize: 15
                }
            }
        }

        Text {
            id: errorLabel
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: passwordBox.bottom
            anchors.topMargin: 8
            width: parent.width - 48
            height: 18
            horizontalAlignment: Text.AlignHCenter
            text: lock.authenticating ? qsTr("Comprobando…") : root.errorText
            color: lock.authenticating ? Theme.textSecondary : "#ffb3bd"
            font.family: Theme.fontFamily
            font.pixelSize: 12
            elide: Text.ElideRight
        }

        GlassButton {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: errorLabel.bottom
            anchors.topMargin: 10
            width: 320
            height: 44
            text: lock.authenticating ? qsTr("Desbloqueando…") : qsTr("Desbloquear")
            enabled: !lock.authenticating
            onClicked: root.submit()
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 16
            text: qsTr("Pulsa Enter para desbloquear")
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: 10
        }

        SequentialAnimation {
            id: shake

            NumberAnimation {
                target: card
                property: "anchors.horizontalCenterOffset"
                to: -12
                duration: 45
            }
            NumberAnimation {
                target: card
                property: "anchors.horizontalCenterOffset"
                to: 12
                duration: 60
            }
            NumberAnimation {
                target: card
                property: "anchors.horizontalCenterOffset"
                to: -7
                duration: 55
            }
            NumberAnimation {
                target: card
                property: "anchors.horizontalCenterOffset"
                to: 7
                duration: 50
            }
            NumberAnimation {
                target: card
                property: "anchors.horizontalCenterOffset"
                to: 0
                duration: 45
            }
        }
    }

    Connections {
        target: lock

        function onAuthenticationFailed(message) {
            root.errorText = message
            passwordInput.text = ""
            passwordInput.forceActiveFocus()
            shake.restart()
        }
    }

    // Evita atajos del sistema que puedan cerrar o tapar la superficie.
    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Escape) {
            event.accepted = true
        }
    }
}
