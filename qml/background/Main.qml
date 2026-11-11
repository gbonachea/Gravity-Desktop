import QtQuick
import "../common"
import "../common/Theme.js" as Theme

/*!
 * Bottom-most layer-shell surface. It paints the selected wallpaper with a
 * cover fit, draws the desktop shortcuts and hosts the desktop context menu.
 * Keeping the icons here means they live exactly on the wallpaper surface,
 * independently of the file manager.
 */
Item {
    id: root

    width: 1920
    height: 1080

    property int menuRow: -1
    property int hoveredIcon: -1
    property int selectedIcon: -1

    // Fallback sky/horizon gradient, close to the XP "Bliss" mood.
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#0a2a5e" }
            GradientStop { position: 0.48; color: "#1f65b8" }
            GradientStop { position: 0.76; color: "#4f9bd8" }
            GradientStop { position: 1.0; color: "#8fd0f0" }
        }
    }

    Image {
        id: photo
        anchors.fill: parent
        source: wallpaper.source
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        cache: true
        smooth: true
        mipmap: true
        visible: status === Image.Ready
        opacity: status === Image.Ready ? 1.0 : 0.0

        Behavior on opacity { NumberAnimation { duration: Theme.slow } }
    }

    // Subtle top/bottom shading so the acrylic panel and the menu keep
    // contrast over bright wallpapers.
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#33040f22" }
            GradientStop { position: 0.28; color: "#00040f22" }
            GradientStop { position: 0.72; color: "#00040f22" }
            GradientStop { position: 1.0; color: "#4d081733" }
        }
    }

    // Soft highlight from the upper-left, matching the glass components.
    Rectangle {
        anchors.fill: parent
        opacity: 0.16
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: "#66ffffff" }
            GradientStop { position: 0.45; color: "#00ffffff" }
        }
    }

    // ------------------------------------------------------------------
    // Escritorio: accesos directos / iconos
    // ------------------------------------------------------------------
    GridView {
        id: desktopGrid
        anchors.fill: parent
        anchors.margins: 18
        anchors.topMargin: (typeof settings !== "undefined" && settings
                           && settings.panelPosition === "top")
            ? settings.panelHeight + 18 : 18
        anchors.bottomMargin: (typeof settings !== "undefined" && settings
                               && settings.panelPosition === "top")
            ? 26 : settings.panelHeight + 26
        cellWidth: 106
        cellHeight: 116
        flow: GridView.FlowTopToBottom
        interactive: false
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        model: desktopIcons

        delegate: DesktopIcon {
            name: model.name
            iconName: model.icon
            kind: model.kind
            removable: model.removable
            hovered: root.hoveredIcon === index
            selected: root.selectedIcon === index
        }
    }

    // Capa única de ratón sobre la rejilla: selecciona con un clic, abre con
    // doble clic y muestra el menú contextual con el botón derecho. Al
    // gestionar todo desde aquí el doble clic es fiable, sin depender de los
    // delegados del GridView.
    MouseArea {
        id: desktopMouse
        anchors.fill: parent
        z: 2
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        cursorShape: root.hoveredIcon >= 0 ? Qt.PointingHandCursor : Qt.ArrowCursor

        function iconAt(x, y) {
            const point = desktopGrid.mapFromItem(root, x, y)
            return desktopGrid.indexAt(point.x, point.y)
        }

        onClicked: function(mouse) {
            if (mouse.button === Qt.RightButton) {
                if (desktopMenu.visible) {
                    desktopMenu.close()
                    return
                }
                const index = iconAt(mouse.x, mouse.y)
                if (index >= 0) {
                    root.selectedIcon = index
                    root.showIconMenu(index, mouse.x, mouse.y)
                } else {
                    root.selectedIcon = -1
                    root.showDesktopMenu(mouse.x, mouse.y)
                }
                return
            }

            root.selectedIcon = iconAt(mouse.x, mouse.y)
        }

        onDoubleClicked: function(mouse) {
            if (mouse.button !== Qt.LeftButton) {
                return
            }
            const index = iconAt(mouse.x, mouse.y)
            if (index >= 0) {
                root.selectedIcon = index
                desktopIcons.open(index)
            }
        }

        onPositionChanged: function(mouse) {
            root.hoveredIcon = iconAt(mouse.x, mouse.y)
        }

        onExited: root.hoveredIcon = -1
    }

    // Cierra el menú al hacer clic fuera de él.
    MouseArea {
        anchors.fill: parent
        z: 90
        visible: desktopMenu.visible
        enabled: desktopMenu.visible
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: desktopMenu.close()
    }

    DesktopMenu {
        id: desktopMenu
        desktopRoot: root
        onActionTriggered: function(action) { root.handleMenuAction(action) }
    }

    // Aviso breve para errores al abrir o quitar accesos.
    Rectangle {
        id: toast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: (typeof settings !== "undefined" && settings
                               && settings.panelPosition === "bottom")
            ? settings.panelHeight + 28 : 28
        width: Math.min(420, toastText.implicitWidth + 28)
        height: 38
        radius: 10
        color: "#ec0b2452"
        border.width: 1
        border.color: "#66cfe7ff"
        opacity: 0.0
        visible: opacity > 0.01
        z: 120

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
            color: "#e8f3ff"
            font.family: Theme.fontFamily
            font.pixelSize: 12
            font.weight: Font.DemiBold
        }

        Timer {
            id: toastTimer
            interval: 2600
            onTriggered: toast.opacity = 0.0
        }

        Behavior on opacity { NumberAnimation { duration: Theme.fast } }
    }

    Connections {
        target: desktopIcons
        function onActionFailed(message) { toast.show(message) }
    }

    Connections {
        target: systemActions
        function onFailed(message) { toast.show(message) }
    }

    // ------------------------------------------------------------------
    // Menús
    // ------------------------------------------------------------------
    function showDesktopMenu(x, y) {
        root.menuRow = -1
        desktopMenu.openAt(x, y, [
            { label: qsTr("Nueva carpeta"), icon: "folder-new", action: "new-folder" },
            { label: qsTr("Abrir carpeta del escritorio"),
              icon: "folder-open", action: "open-desktop" },
            { label: qsTr("Abrir terminal aquí"),
              icon: "utilities-terminal", action: "terminal" },
            { separator: true },
            { label: qsTr("Actualizar"), icon: "view-refresh", action: "refresh" },
            { label: qsTr("Centro de configuración"),
              icon: "preferences-system", action: "control-panel" }
        ])
    }

    function showIconMenu(index, x, y) {
        const item = desktopIcons.itemAt(index)
        if (!item || item.name === undefined) {
            root.showDesktopMenu(x, y)
            return
        }
        root.menuRow = index

        let items = [
            { label: qsTr("Abrir"), icon: item.icon, action: "open" }
        ]
        if (item.kind === "folder" || item.kind === "file") {
            items.push({ label: qsTr("Abrir en terminal"),
                         icon: "utilities-terminal", action: "terminal" })
        }
        if (item.removable) {
            let label = qsTr("Quitar del escritorio")
            if (item.removeKind === "trashfile") {
                label = qsTr("Mover a la papelera")
            } else if (item.kind === "special") {
                label = qsTr("Ocultar del escritorio")
            }
            items.push({ separator: true })
            items.push({ label: label, icon: "edit-delete",
                         action: "remove", danger: true })
        }
        desktopMenu.openAt(x, y, items)
    }

    function handleMenuAction(action) {
        switch (action) {
        case "open":
            desktopIcons.open(root.menuRow)
            break
        case "terminal":
            desktopIcons.openTerminal(root.menuRow)
            break
        case "remove":
            desktopIcons.remove(root.menuRow)
            break
        case "new-folder":
            desktopIcons.createFolder()
            break
        case "open-desktop":
            desktopIcons.openDesktopFolder()
            break
        case "refresh":
            desktopIcons.reload()
            break
        case "control-panel":
            systemActions.openControlPanel()
            break
        }
        root.menuRow = -1
        root.selectedIcon = -1
    }
}
