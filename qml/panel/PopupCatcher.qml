import QtQuick

Item {
    anchors.fill: parent

    MouseArea {
        anchors.fill: parent
        onClicked: popup.close()
    }
}