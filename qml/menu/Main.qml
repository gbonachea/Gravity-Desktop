import QtQuick
import QtQuick.Effects
import "../common"
import "../common/Theme.js" as Theme

Item {
    id: root

    width: 680
    height: 600
    focus: true

    opacity: menuWindow.visible ? 1.0 : 0.0
    scale: menuWindow.visible ? 1.0 : 0.965
    transformOrigin: Item.BottomLeft

    Behavior on opacity { NumberAnimation { duration: Theme.normal } }
    Behavior on scale { NumberAnimation { duration: Theme.slow; easing.type: Easing.OutCubic } }

    property bool expanded: false
    property var quickActions: [
        { label: qsTr("Mis documentos"), glyph: "▤", icon: "folder-documents", color: "#59aaff", action: "documents" },
        { label: qsTr("Mi PC"), glyph: "▥", icon: "computer", color: "#42c46e", action: "computer" },
        { label: qsTr("Panel de control"), glyph: "⚙", icon: "preferences-system", color: "#ffb84d", action: "control" },
        { label: qsTr("Configuración"), glyph: "☰", icon: "preferences-other", color: "#b48cff", action: "settings" },
        { label: qsTr("Buscar"), glyph: "⌕", icon: "system-search", color: "#58d6e8", action: "search" },
        { label: qsTr("Red y sonido"), glyph: "⌁", icon: "preferences-system-network", color: "#58d6e8", action: "system" },
        { label: qsTr("Ejecutar..."), glyph: "▶", icon: "system-run", color: "#ef7b8a", action: "run" }
    ]

    function updateLimit() {
        apps.maxEntries = (expanded || search.text.length > 0) ? -1 : 11
    }

    function focusSearch() {
        search.forceInputFocus()
    }

    function handleQuickAction(action) {
        switch (action) {
        case "documents":
            systemActions.openDocuments()
            menuWindow.hideMenu()
            break
        case "computer":
            systemActions.openComputer()
            menuWindow.hideMenu()
            break
        case "control":
            systemActions.openControlPanel()
            menuWindow.hideMenu()
            break
        case "settings":
            systemActions.openSettings()
            menuWindow.hideMenu()
            break
        case "search":
            focusSearch()
            break
        case "system":
            systemPanel.open()
            break
        case "run":
            runDialog.open()
            break
        }
    }

    Keys.onEscapePressed: {
        if (contextMenu.visible) {
            contextMenu.close()
        } else if (systemPanel.opened) {
            systemPanel.close()
        } else if (runDialog.opened) {
            runDialog.close()
        } else {
            menuWindow.hideMenu()
        }
    }

    Connections {
        target: menuWindow

        function onShown() {
            root.forceActiveFocus()
        }

        function onHidden() {
            search.text = ""
            apps.filterText = ""
            root.expanded = false
            root.updateLimit()
            runDialog.close()
            systemPanel.close()
            contextMenu.close()
        }
    }

    Connections {
        target: launcher
        function onLaunchFailed(message) { toast.show(message) }
    }

    Connections {
        target: systemActions
        function onFailed(message) { toast.show(message) }
    }

    Connections {
        target: system
        function onErrorOccurred(message) { toast.show(message) }
        function onActionStarted(action) { toast.show(qsTr("%1 en curso…").arg(action)) }
    }

    GlassCard {
        id: background
        anchors.fill: parent
        radius: Theme.radius
        tintTop: Theme.menuTop
        tintBottom: Theme.menuBottom
        borderColor: Theme.menuBorder
        shadowOffset: 8
    }

    // ---------------------------------------------------------------- header
    Item {
        id: header
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: 20
        anchors.rightMargin: 20
        height: 84

        Item {
            id: avatar
            width: 52
            height: 52
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter

            Rectangle {
                anchors.fill: parent
                radius: width / 2
                border.width: 2
                border.color: "#99d6efff"
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#8dc4ff" }
                    GradientStop { position: 1.0; color: "#1f5fc9" }
                }

                Image {
                    id: faceSource
                    anchors.fill: parent
                    anchors.margins: 2
                    source: session.avatarSource
                    fillMode: Image.PreserveAspectCrop
                    visible: false
                    layer.enabled: true
                }

                MultiEffect {
                    anchors.fill: parent
                    anchors.margins: 2
                    visible: session.avatarSource.length > 0
                    source: faceSource
                    maskEnabled: true
                    maskSource: faceMask
                }

                Rectangle {
                    id: faceMask
                    anchors.fill: parent
                    anchors.margins: 2
                    radius: width / 2
                    color: "#ffffff"
                    visible: false
                    layer.enabled: true
                }

                Text {
                    anchors.centerIn: parent
                    visible: session.avatarSource.length === 0
                    text: session.initials
                    color: "#ffffff"
                    font.family: Theme.fontFamily
                    font.pixelSize: 19
                    font.weight: Font.DemiBold
                }
            }
        }

        Column {
            anchors.left: avatar.right
            anchors.leftMargin: 14
            anchors.right: search.left
            anchors.rightMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            spacing: 3

            Text {
                width: parent.width
                text: session.displayName
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: 18
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }

            Text {
                width: parent.width
                text: session.sessionLabel
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: 10
                elide: Text.ElideRight
            }
        }

        SearchField {
            id: search
            width: 238
            height: 36
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            placeholder: qsTr("Buscar programas...")
            onTextChanged: {
                apps.filterText = text
                root.updateLimit()
            }
            onEscapePressed: menuWindow.hideMenu()
        }
    }

    Rectangle {
        id: headerDivider
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        anchors.leftMargin: 20
        anchors.rightMargin: 20
        height: 1
        color: Theme.divider
    }

    // ------------------------------------------------------------------ body
    Row {
        id: body
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: headerDivider.bottom
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.topMargin: 12
        height: 432
        spacing: 12

        // Left: applications
        Rectangle {
            id: leftColumn
            width: 416
            height: parent.height
            radius: 12
            color: "#12ffffff"
            border.width: 1
            border.color: "#24ffffff"

            Text {
                id: appsLabel
                x: 14
                y: 12
                text: qsTr("APLICACIONES")
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: 10
                font.letterSpacing: 1.2
                font.weight: Font.DemiBold
            }

            Text {
                anchors.right: parent.right
                anchors.rightMargin: 14
                y: 12
                text: apps.count
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: 10
            }

            ListView {
                id: appList
                x: 8
                y: 32
                width: parent.width - 16
                height: parent.height - 32 - 52
                clip: true
                model: apps
                spacing: 1
                boundsBehavior: Flickable.StopAtBounds
                flickableDirection: Flickable.VerticalFlick
                cacheBuffer: 400

                delegate: AppListDelegate {
                    name: model.name
                    iconName: model.icon
                    comment: model.comment
                    desktopFile: model.desktopFile
                    onActivated: {
                        if (launcher.launchDesktopFile(desktopFile)) {
                            menuWindow.hideMenu()
                        }
                    }
                    onContextMenuRequested: function(file, appName, sx, sy) {
                        contextMenu.open(file, appName, sx, sy)
                    }
                }

                Rectangle {
                    visible: appList.contentHeight > appList.height
                    width: 3
                    radius: 1.5
                    color: "#4dffffff"
                    x: appList.width - 4
                    height: Math.max(24, appList.height * appList.visibleArea.heightRatio)
                    y: appList.height * appList.visibleArea.yPosition

                    Behavior on y { NumberAnimation { duration: 60 } }
                }

                Text {
                    anchors.centerIn: parent
                    visible: apps.count === 0
                    text: (search.text.length > 0 || root.expanded)
                        ? qsTr("No se encontraron aplicaciones")
                        : qsTr("No hay aplicaciones favoritas")
                    color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: 11
                }
            }

            GlassButton {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                anchors.bottomMargin: 8
                height: 40
                baseColor: "#1affffff"
                hoverColor: "#30ffffff"
                text: root.expanded ? qsTr("Mostrar menos  ‹") : qsTr("Todos los programas  ›")
                onClicked: {
                    root.expanded = !root.expanded
                    root.updateLimit()
                }
            }
        }

        // Right: quick links
        Rectangle {
            id: rightColumn
            width: 220
            height: parent.height
            radius: 12
            color: "#12ffffff"
            border.width: 1
            border.color: "#24ffffff"

            Text {
                x: 14
                y: 12
                text: qsTr("ACCESOS RÁPIDOS")
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: 10
                font.letterSpacing: 1.0
                font.weight: Font.DemiBold
            }

            Column {
                x: 8
                y: 32
                width: parent.width - 16
                spacing: 2

                Repeater {
                    model: root.quickActions

                    QuickLink {
                        width: parent.width
                        label: modelData.label
                        glyph: modelData.glyph
                        iconName: modelData.icon !== undefined ? modelData.icon : ""
                        glyphColor: modelData.color
                        onClicked: root.handleQuickAction(modelData.action)
                    }
                }
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 10
                height: 76
                radius: 10
                color: "#10ffffff"
                border.width: 1
                border.color: "#20ffffff"

                Row {
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 8

                    XpFlag {
                        width: 30
                        height: 30
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 2

                        Text {
                            text: qsTr("Stellar Shell")
                            color: Theme.textPrimary
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                        }

                        Text {
                            text: qsTr("Edición XP · Remasterizada")
                            color: Theme.textMuted
                            font.family: Theme.fontFamily
                            font.pixelSize: 9
                        }
                    }
                }

                Rectangle {
                    width: 8
                    height: 8
                    radius: 4
                    color: Theme.xpGreen
                    anchors.right: parent.right
                    anchors.rightMargin: 10
                    anchors.top: parent.top
                    anchors.topMargin: 10
                }
            }
        }
    }

    // ---------------------------------------------------------------- footer
    Rectangle {
        id: footerDivider
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: footer.top
        anchors.leftMargin: 20
        anchors.rightMargin: 20
        height: 1
        color: Theme.divider
    }

    Item {
        id: footer
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 20
        anchors.rightMargin: 20
        anchors.bottomMargin: 10
        height: 48

        Row {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8

            Text {
                text: "✦"
                color: Theme.textMuted
                font.pixelSize: 13
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: qsTr("Sesión Wayland")
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: 10
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        Row {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            spacing: 6

            PowerButton {
                glyph: "⌾"
                label: qsTr("Bloquear")
                onClicked: {
                    menuWindow.hideMenu()
                    system.lock()
                }
            }

            PowerButton {
                glyph: "⏏"
                label: qsTr("Cerrar sesión")
                onClicked: {
                    menuWindow.hideMenu()
                    system.logout()
                }
            }

            PowerButton {
                glyph: "☾"
                label: qsTr("Suspender")
                onClicked: system.suspend()
            }

            PowerButton {
                glyph: "↻"
                label: qsTr("Reiniciar")
                onClicked: system.reboot()
            }

            PowerButton {
                glyph: "⏻"
                label: qsTr("Apagar")
                danger: true
                onClicked: system.powerOff()
            }
        }
    }

    // ----------------------------------------------------------------- toast
    Rectangle {
        id: toast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 70
        width: Math.min(parent.width - 48, toastText.implicitWidth + 34)
        height: 36
        radius: 10
        color: "#dd6f1f32"
        border.width: 1
        border.color: "#66ff9aa8"
        opacity: 0.0
        visible: opacity > 0.01
        z: 50

        function show(message) {
            toastText.text = message
            toast.opacity = 1.0
            toastTimer.restart()
        }

        Text {
            id: toastText
            anchors.centerIn: parent
            width: parent.width - 20
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
            color: "#ffe8ec"
            font.family: Theme.fontFamily
            font.pixelSize: 11
        }

        Timer {
            id: toastTimer
            interval: 3800
            onTriggered: toast.opacity = 0.0
        }

        Behavior on opacity { NumberAnimation { duration: Theme.normal } }
    }

    SystemPanel {
        id: systemPanel
        anchors.fill: parent
    }

    RunDialog {
        id: runDialog
        anchors.fill: parent
        onCommandRequested: function(command) {
            if (launcher.launchCommand(command)) {
                menuWindow.hideMenu()
            }
        }
    }

    // Menú contextual de aplicaciones (clic derecho en la lista).
    Item {
        id: contextMenu
        anchors.fill: parent
        visible: false
        z: 200

        property string file: ""
        property string appName: ""

        function open(fileName, appLabel, sceneX, sceneY) {
            file = fileName
            appName = appLabel
            menuCard.x = Math.min(Math.max(8, sceneX + 6),
                                  parent.width - menuCard.width - 8)
            menuCard.y = Math.min(Math.max(8, sceneY + 6),
                                  parent.height - menuCard.height - 8)
            visible = true
        }

        function close() {
            visible = false
        }

        MouseArea {
            anchors.fill: parent
            onClicked: contextMenu.close()
        }

        GlassCard {
            id: menuCard
            width: 270
            height: 108
            radius: 12
            tintTop: Theme.menuTop
            tintBottom: Theme.menuBottom
            borderColor: Theme.menuBorder
            shadowOffset: 8

            MouseArea { anchors.fill: parent }

            Column {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 6

                GlassButton {
                    width: parent.width
                    height: 40
                    text: settings.isFavorite(contextMenu.file)
                        ? qsTr("Quitar de Favoritos")
                        : qsTr("Agregar a Favoritos")
                    onClicked: {
                        settings.toggleFavorite(contextMenu.file)
                        contextMenu.close()
                    }
                }

                GlassButton {
                    width: parent.width
                    height: 40
                    text: qsTr("Anclar en el escritorio")
                    onClicked: {
                        settings.pinToDesktop(contextMenu.file)
                        contextMenu.close()
                    }
                }
            }
        }
    }
}
