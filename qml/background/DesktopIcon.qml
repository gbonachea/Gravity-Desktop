import QtQuick
import "../common"
import "../common/Theme.js" as Theme

/*!
 * Icono individual del escritorio. Se dibuja sobre la capa Background; la
 * interacción (selección, doble clic y menú contextual) la gestiona la capa
 * de ratón de Main.qml, de modo que aquí solo se pinta el estado visual.
 */
Item {
    id: root

    property string name: ""
    property string iconName: "application-x-executable"
    property string kind: "file"
    property bool removable: false
    property bool hovered: false
    property bool selected: false

    width: 104
    height: 114

    Rectangle {
        anchors.fill: parent
        radius: 10
        color: root.selected ? "#3d2f7fd0"
                          : root.hovered ? "#2bffffff" : "transparent"
        border.width: root.selected || root.hovered ? 1 : 0
        border.color: root.selected ? "#8cc8f0ff" : "#4dbfe3ff"

        Behavior on color { ColorAnimation { duration: Theme.fast } }
    }

    StellarIcon {
        id: icon
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 8
        size: 50
        showTile: false
        iconName: root.iconName
    }

    Text {
        anchors.top: icon.bottom
        anchors.topMargin: 6
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width - 12
        text: root.name
        color: "#f4f9ff"
        style: Text.Outline
        styleColor: "#bb00162e"
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        maximumLineCount: 2
        elide: Text.ElideRight
        font.family: Theme.fontFamily
        font.pixelSize: 11
        font.weight: Font.DemiBold
    }
}
