import QtQuick
import "../common/Theme.js" as Theme

Item {
    id: root

    width: 360
    height: 520

    Column {
        id: cards
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: 16

        Repeater {
            model: settings.visibleWidgets

            delegate: Loader {
                width: cards.width
                sourceComponent: modelData === "clock" ? clockCard
                    : modelData === "storage" ? storageCard
                    : statusCard
            }
        }
    }

    Component {
        id: clockCard
        ClockWeatherCard {
            width: cards.width
            height: 176
        }
    }

    Component {
        id: storageCard
        StorageCard {
            width: cards.width
            height: 150
        }
    }

    Component {
        id: statusCard
        SystemStatusCard {
            width: cards.width
            height: 150
        }
    }

    opacity: 0.0
    transform: Translate { id: slide; x: 18 }

    Component.onCompleted: intro.start()

    ParallelAnimation {
        id: intro
        NumberAnimation {
            target: root
            property: "opacity"
            from: 0.0
            to: 1.0
            duration: 520
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            target: slide
            property: "x"
            from: 18
            to: 0
            duration: 520
            easing.type: Easing.OutCubic
        }
    }
}