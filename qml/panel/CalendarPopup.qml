import QtQuick
import "../common"
import "../common/Theme.js" as Theme

/*!
 * Vista de calendario del panel. Se muestra dentro del popup del reloj y usa
 * la fecha local del sistema, con la semana empezando según el idioma.
 */
Item {
    id: root

    property date today: new Date()
    property int shownYear: new Date().getFullYear()
    property int shownMonth: new Date().getMonth()
    readonly property int firstDayOfWeek: Qt.locale().firstDayOfWeek
    readonly property var cells: buildCells()
    readonly property int gridWidth: width

    function capitalize(text) {
        return text.length > 0 ? text.charAt(0).toUpperCase() + text.slice(1) : text
    }

    function isSameDay(a, b) {
        return a.getFullYear() === b.getFullYear()
            && a.getMonth() === b.getMonth()
            && a.getDate() === b.getDate()
    }

    function buildCells() {
        const first = new Date(shownYear, shownMonth, 1)
        // En QML firstDayOfWeek es 0 = domingo … 6 = sábado, igual que el
        // getDay() de JavaScript.
        const offset = (first.getDay() - firstDayOfWeek + 7) % 7
        const start = new Date(shownYear, shownMonth, 1 - offset)
        const now = today
        const result = []
        for (let i = 0; i < 42; ++i) {
            const day = new Date(start.getFullYear(), start.getMonth(), start.getDate() + i)
            result.push({
                day: day.getDate(),
                inMonth: day.getMonth() === shownMonth,
                isToday: isSameDay(day, now)
            })
        }
        return result
    }

    function shiftMonth(delta) {
        const shifted = new Date(shownYear, shownMonth + delta, 1)
        shownYear = shifted.getFullYear()
        shownMonth = shifted.getMonth()
    }

    function goToday() {
        const now = new Date()
        today = now
        shownYear = now.getFullYear()
        shownMonth = now.getMonth()
    }

    function weekdayLabel(index) {
        // En QML dayName usa 0 = domingo … 6 = sábado.
        const day = (firstDayOfWeek + index) % 7
        return Qt.locale().dayName(day, Locale.ShortFormat).replace(/\.$/, "")
    }

    Timer {
        interval: 60000
        running: true
        repeat: true
        onTriggered: {
            const now = new Date()
            if (!isSameDay(now, root.today)) {
                root.today = now
            }
        }
    }

    Column {
        anchors.fill: parent
        spacing: 8

        // Hora y fecha actuales.
        Row {
            width: parent.width
            spacing: 12

            Text {
                text: clock.time
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: 36
                font.weight: Font.Light
                anchors.verticalCenter: parent.verticalCenter
            }

            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 1

                Text {
                    text: clock.weekday
                    color: Theme.textPrimary
                    font.family: Theme.fontFamily
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                }

                Text {
                    text: clock.dateText
                    color: Theme.textSecondary
                    font.family: Theme.fontFamily
                    font.pixelSize: 11
                }
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: Theme.divider
        }

        // Navegación de mes.
        Row {
            width: parent.width
            spacing: 8

            GlassButton {
                width: 38
                height: 30
                text: "‹"
                baseColor: "#16ffffff"
                hoverColor: "#30ffffff"
                onClicked: root.shiftMonth(-1)
            }

            Text {
                width: parent.width - 160
                height: 30
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: root.capitalize(Qt.locale().monthName(root.shownMonth,
                                                            Locale.LongFormat))
                      + " " + root.shownYear
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: 15
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }

            GlassButton {
                width: 38
                height: 30
                text: "›"
                baseColor: "#16ffffff"
                hoverColor: "#30ffffff"
                onClicked: root.shiftMonth(1)
            }

            GlassButton {
                width: 60
                height: 30
                text: qsTr("Hoy")
                baseColor: "#16ffffff"
                hoverColor: "#30ffffff"
                onClicked: root.goToday()
            }
        }

        // Cabecera de días de la semana.
        Row {
            width: parent.width

            Repeater {
                model: 7

                Text {
                    width: root.gridWidth / 7
                    height: 18
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    text: root.weekdayLabel(index)
                    color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: 10
                    font.weight: Font.DemiBold
                }
            }
        }

        // Rejilla de 6 semanas.
        Grid {
            id: dayGrid
            width: parent.width
            columns: 7

            Repeater {
                model: root.cells

                Item {
                    width: dayGrid.width / 7
                    height: 36

                    Rectangle {
                        anchors.centerIn: parent
                        width: 32
                        height: 32
                        radius: 16
                        color: modelData.isToday
                            ? "#3f8fe0"
                            : (dayMouse.containsMouse ? "#26ffffff" : "transparent")
                        border.width: modelData.isToday ? 1 : 0
                        border.color: "#a7d6ff"

                        Behavior on color { ColorAnimation { duration: Theme.fast } }
                    }

                    Text {
                        anchors.centerIn: parent
                        text: modelData.day
                        color: modelData.inMonth
                            ? (modelData.isToday ? "#ffffff" : Theme.textPrimary)
                            : "#6f8aa8"
                        font.family: Theme.fontFamily
                        font.pixelSize: 12
                        font.weight: modelData.isToday ? Font.DemiBold : Font.Normal
                    }

                    MouseArea {
                        id: dayMouse
                        anchors.fill: parent
                        hoverEnabled: true
                    }
                }
            }
        }

        Item { width: 1; height: 1 }

        Text {
            width: parent.width
            text: qsTr("Haz clic fuera del calendario para cerrarlo")
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: 10
            horizontalAlignment: Text.AlignHCenter
        }
    }
}
