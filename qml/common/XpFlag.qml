import QtQuick

/*!
 * Stylized four-pane flag inspired by the classic XP logo, drawn entirely
 * with QML so no trademarked bitmap asset is required.
 */
Item {
    id: root

    implicitWidth: 30
    implicitHeight: 30

    Rectangle {
        anchors.fill: parent
        radius: width * 0.24
        color: "#f6fbff"
        border.width: 1
        border.color: "#9fc4e8"
    }

    Item {
        anchors.centerIn: parent
        width: parent.width * 0.72
        height: parent.height * 0.72
        rotation: -4

        Grid {
            anchors.fill: parent
            columns: 2
            columnSpacing: Math.max(1, root.width * 0.045)
            rowSpacing: Math.max(1, root.height * 0.045)

            Repeater {
                model: ["#ef5350", "#8bc34a", "#42a5f5", "#ffd54f"]

                Rectangle {
                    width: (parent.width - parent.columnSpacing) / 2
                    height: (parent.height - parent.rowSpacing) / 2
                    radius: Math.max(1.5, width * 0.18)
                    color: modelData
                    border.width: 0.5
                    border.color: "#33000000"
                }
            }
        }
    }
}
