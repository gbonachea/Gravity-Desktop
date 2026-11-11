import QtQuick
import "Theme.js" as Theme

Item {
    id: root

    property string iconName: "application-x-executable"
    property int size: 32
    property bool showTile: true
    property color tileColor: "#1fffffff"
    property color tileBorder: "#33ffffff"
    readonly property string themeKey: (typeof settings !== "undefined" && settings)
        ? String(settings.iconTheme) : ""

    implicitWidth: size
    implicitHeight: size

    Rectangle {
        anchors.fill: parent
        radius: root.size * 0.28
        visible: root.showTile
        color: root.tileColor
        border.width: 1
        border.color: root.tileBorder
    }

    Image {
        anchors.centerIn: parent
        width: root.showTile ? root.size * 0.72 : root.size
        height: width
        source: root.iconName.length > 0
            ? "image://icon/" + encodeURIComponent(root.iconName)
                + (root.themeKey.length > 0 ? "?theme=" + encodeURIComponent(root.themeKey) : "")
            : ""
        sourceSize.width: root.size * 2
        sourceSize.height: root.size * 2
        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: true
        asynchronous: true
    }
}
