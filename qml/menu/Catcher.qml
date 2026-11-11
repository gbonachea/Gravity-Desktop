import QtQuick

/*!
 * Transparent full-screen surface shown under the start menu while it is open.
 * Layer-shell surfaces do not get a popup grab, so this surface captures any
 * click outside the menu and asks the menu controller to dismiss it.
 */
Item {
    id: root

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.AllButtons
        onPressed: menuWindow.hideMenu()
        onWheel: menuWindow.hideMenu()
    }
}
