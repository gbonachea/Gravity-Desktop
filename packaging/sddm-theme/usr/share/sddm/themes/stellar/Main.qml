import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    width: 1920
    height: 1080

    gradient: Gradient {
        GradientStop { position: 0.0; color: "#071a33" }
        GradientStop { position: 0.55; color: "#0d2f5e" }
        GradientStop { position: 1.0; color: "#071226" }
    }

    // Adornos de fondo
    Rectangle {
        width: parent.width * 0.55
        height: width
        radius: width / 2
        x: -width * 0.25
        y: -height * 0.35
        color: "#1a4fa0"
        opacity: 0.22
    }
    Rectangle {
        width: parent.width * 0.45
        height: width
        radius: width / 2
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: -width * 0.2
        anchors.bottomMargin: -height * 0.3
        color: "#2a7cf0"
        opacity: 0.16
    }

    Column {
        anchors.centerIn: parent
        spacing: 18

        // Tarjeta principal
        Rectangle {
            width: 440
            height: 440
            radius: 22
            color: "#d0174b8f"
            border.width: 1
            border.color: "#8fcfe7ff"

            // Gloss superior
            Rectangle {
                anchors.fill: parent
                anchors.margins: 1
                radius: parent.radius - 1
                opacity: 0.55
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#40ffffff" }
                    GradientStop { position: 0.3; color: "#00ffffff" }
                }
            }

            Column {
                anchors.fill: parent
                anchors.margins: 34
                spacing: 16

                Row {
                    spacing: 12
                    anchors.horizontalCenter: parent.horizontalCenter

                    // Bandera tipo XP
                    Item {
                        width: 46
                        height: 46
                        anchors.verticalCenter: parent.verticalCenter

                        Grid {
                            anchors.fill: parent
                            columns: 2
                            spacing: 4
                            Rectangle { width: 21; height: 21; radius: 3; color: "#ff6b6b" }
                            Rectangle { width: 21; height: 21; radius: 3; color: "#ffd54f" }
                            Rectangle { width: 21; height: 21; radius: 3; color: "#59aaff" }
                            Rectangle { width: 21; height: 21; radius: 3; color: "#42c46e" }
                        }
                    }

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 0
                        Text {
                            text: "Stellar Shell"
                            color: "#ffffff"
                            font.family: "sans-serif"
                            font.pixelSize: 30
                            font.weight: Font.DemiBold
                        }
                        Text {
                            text: sddm.hostName || "Stellar"
                            color: "#c6dcf7"
                            font.family: "sans-serif"
                            font.pixelSize: 13
                        }
                    }
                }

                Text {
                    text: qsTr("Inicia sesión")
                    color: "#e8f2ff"
                    font.family: "sans-serif"
                    font.pixelSize: 15
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                TextField {
                    id: userField
                    width: parent.width
                    height: 44
                    text: userModel.lastUser
                    placeholderText: qsTr("Usuario")
                    color: "#ffffff"
                    placeholderTextColor: "#a9c4e8"
                    font.family: "sans-serif"
                    font.pixelSize: 15
                    leftPadding: 16
                    background: Rectangle {
                        radius: 12
                        color: "#2a4a74"
                        border.width: 1
                        border.color: userField.activeFocus ? "#59aaff" : "#4f7ab0"
                    }
                    Keys.onReturnPressed: passwordField.forceActiveFocus()
                }

                TextField {
                    id: passwordField
                    width: parent.width
                    height: 44
                    echoMode: TextInput.Password
                    placeholderText: qsTr("Contraseña")
                    color: "#ffffff"
                    placeholderTextColor: "#a9c4e8"
                    font.family: "sans-serif"
                    font.pixelSize: 15
                    leftPadding: 16
                    background: Rectangle {
                        radius: 12
                        color: "#2a4a74"
                        border.width: 1
                        border.color: passwordField.activeFocus ? "#59aaff" : "#4f7ab0"
                    }
                    Keys.onReturnPressed: root.tryLogin()
                }

                ComboBox {
                    id: sessionBox
                    width: parent.width
                    height: 40
                    model: sessionModel
                    textRole: "name"
                    currentIndex: sessionModel.lastIndex >= 0 ? sessionModel.lastIndex : 0
                    font.family: "sans-serif"
                    font.pixelSize: 13
                }

                Text {
                    id: errorText
                    width: parent.width
                    text: ""
                    color: "#ff9aa8"
                    font.family: "sans-serif"
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                }

                Button {
                    id: loginButton
                    width: parent.width
                    height: 46
                    text: qsTr("Entrar")
                    font.family: "sans-serif"
                    font.pixelSize: 16
                    font.weight: Font.DemiBold

                    background: Rectangle {
                        radius: 12
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#59aaff" }
                            GradientStop { position: 1.0; color: "#2a7cf0" }
                        }
                        border.width: 1
                        border.color: "#9fd0ff"
                    }

                    contentItem: Text {
                        text: loginButton.text
                        color: "#ffffff"
                        font: loginButton.font
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: root.tryLogin()
                }

                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 10

                    Button {
                        visible: sddm.canSuspend
                        text: qsTr("Suspender")
                        onClicked: sddm.suspend()
                    }
                    Button {
                        visible: sddm.canReboot
                        text: qsTr("Reiniciar")
                        onClicked: sddm.reboot()
                    }
                    Button {
                        visible: sddm.canPowerOff
                        text: qsTr("Apagar")
                        onClicked: sddm.powerOff()
                    }
                }
            }
        }
    }

    function tryLogin() {
        if (userField.text.length === 0) {
            errorText.text = qsTr("Introduce el usuario")
            return
        }
        errorText.text = ""
        sddm.login(userField.text, passwordField.text, sessionBox.currentIndex)
    }

    Connections {
        target: sddm
        function onLoginFailed() {
            errorText.text = qsTr("Usuario o contraseña incorrectos")
            passwordField.text = ""
            passwordField.forceActiveFocus()
        }
    }

    Component.onCompleted: {
        if (userField.text.length > 0) {
            passwordField.forceActiveFocus()
        } else {
            userField.forceActiveFocus()
        }
    }
}
