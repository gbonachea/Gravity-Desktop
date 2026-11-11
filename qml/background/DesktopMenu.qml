import QtQuick
import "../common"
import "../common/Theme.js" as Theme

/*!
 * Menú contextual glassmorphism para el escritorio y sus iconos.
 * `entries` es una lista de objetos { label, icon, action, danger, separator }.
 */
Item {
    id: root

    property var entries: []
    property Item desktopRoot: parent

    signal actionTriggered(string action)

    visible: false
    z: 100
    width: 244
    height: content.implicitHeight + 12

    function openAt(x, y, items) {
        entries = items
        visible = true
        const area = desktopRoot ? desktopRoot : root.parent
        const maxX = area ? Math.max(8, area.width - width - 8) : x
        const maxY = area ? Math.max(8, area.height - height - 8) : y
        root.x = Math.max(8, Math.min(x, maxX))
        root.y = Math.max(8, Math.min(y, maxY))
    }

    function close() {
        visible = false
    }

    GlassCard {
        anchors.fill: parent
        radius: 10
        tintTop: "#f21b3c69"
        tintBottom: "#f20d2140"
        borderColor: "#73cfe7ff"
        shadowOffset: 6
    }

    Column {
        id: content
        anchors.fill: parent
        anchors.margins: 6
        spacing: 1

        Repeater {
            model: root.entries

            delegate: Item {
                width: content.width
                height: modelData.separator ? 9 : 32

                Rectangle {
                    visible: modelData.separator === true
                    anchors.verticalCenter: parent.verticalCenter
                    x: 6
                    width: parent.width - 12
                    height: 1
                    color: "#38ffffff"
                }

                Rectangle {
                    visible: modelData.separator !== true && rowMouse.containsMouse
                    anchors.fill: parent
                    radius: 7
                    color: "#33ffffff"
                }

                StellarIcon {
                    visible: modelData.separator !== true && modelData.icon !== undefined
                    anchors.left: parent.left
                    anchors.leftMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    size: 18
                    showTile: false
                    iconName: modelData.icon !== undefined ? modelData.icon : ""
                }

                Text {
                    visible: modelData.separator !== true
                    anchors.left: parent.left
                    anchors.leftMargin: modelData.icon !== undefined ? 34 : 12
                    anchors.right: parent.right
                    anchors.rightMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    text: modelData.label !== undefined ? modelData.label : ""
                    color: modelData.danger ? "#ffb9c1" : Theme.textPrimary
                    font.family: Theme.fontFamily
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }

                MouseArea {
                    id: rowMouse
                    visible: modelData.separator !== true
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        const action = modelData.action
                        root.close()
                        if (action !== undefined) {
                            root.actionTriggered(action)
                        }
                    }
                }
            }
        }
    }
}
