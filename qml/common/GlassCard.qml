import QtQuick
import "Theme.js" as Theme

/*!
 * Frosted acrylic card with a soft drop shadow and a top gloss.
 * Children declared outside are placed in the content item.
 */
Item {
    id: root

    default property alias contentData: contentItem.data

    property real radius: Theme.radius
    property color tintTop: Theme.panelTop
    property color tintBottom: Theme.panelBottom
    property color borderColor: Theme.panelBorder
    property real borderWidth: 1
    property color shadowColor: Theme.shadow
    property bool shadowEnabled: true
    property real shadowOffset: 6
    property real glossOpacity: 0.72

    Rectangle {
        id: shadow
        anchors.fill: parent
        anchors.leftMargin: 2
        anchors.rightMargin: 2
        anchors.topMargin: root.shadowOffset
        radius: root.radius
        color: root.shadowEnabled ? root.shadowColor : "transparent"
        visible: root.shadowEnabled
    }

    Rectangle {
        id: background
        anchors.fill: parent
        radius: root.radius
        border.width: root.borderWidth
        border.color: root.borderColor
        gradient: Gradient {
            GradientStop { position: 0.0; color: root.tintTop }
            GradientStop { position: 1.0; color: root.tintBottom }
        }
    }

    Rectangle {
        anchors.fill: background
        anchors.margins: 1
        radius: Math.max(0, root.radius - 1)
        opacity: root.glossOpacity
        gradient: Gradient {
            GradientStop { position: 0.0; color: Theme.panelHighlight }
            GradientStop { position: 0.26; color: "#00ffffff" }
        }
    }

    Item {
        id: contentItem
        anchors.fill: parent
    }
}
