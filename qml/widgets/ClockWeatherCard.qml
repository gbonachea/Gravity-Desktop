import QtQuick
import "../common"
import "../common/Theme.js" as Theme

GlassCard {
    id: root

    radius: Theme.radius
    tintTop: Theme.lightTop
    tintBottom: Theme.lightBottom
    borderColor: Theme.lightBorder
    shadowColor: Theme.lightShadow
    shadowOffset: 8

    Rectangle {
        width: 150
        height: 150
        radius: width / 2
        x: parent.width - width * 0.55
        y: -height * 0.55
        color: "#33ffffff"
    }

    Text {
        id: bigTime
        x: 18
        y: 6
        text: clock.time
        color: Theme.textOnLight
        font.family: Theme.fontFamily
        font.pixelSize: 46
        font.weight: Font.DemiBold
    }

    Text {
        id: period
        anchors.left: bigTime.right
        anchors.leftMargin: 7
        anchors.bottom: bigTime.bottom
        anchors.bottomMargin: 9
        visible: clock.period.length > 0
        text: clock.period
        color: Theme.textOnLightSecondary
        font.family: Theme.fontFamily
        font.pixelSize: 14
        font.weight: Font.DemiBold
    }

    Text {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 19
        anchors.rightMargin: 18
        y: 64
        text: clock.dateText
        color: Theme.textOnLightSecondary
        font.family: Theme.fontFamily
        font.pixelSize: 11
        elide: Text.ElideRight
    }

    Rectangle {
        id: divider
        x: 18
        y: 90
        width: parent.width - 36
        height: 1
        color: "#33102a49"
    }

    WeatherGlyph {
        id: weatherIcon
        x: 14
        y: 104
        width: 54
        height: 54
        kind: weather.iconKind
    }

    Text {
        id: temperature
        anchors.left: weatherIcon.right
        anchors.leftMargin: 10
        y: 102
        text: weather.temperature
        color: Theme.textOnLight
        font.family: Theme.fontFamily
        font.pixelSize: 27
        font.weight: Font.DemiBold
    }

    Text {
        anchors.left: temperature.left
        anchors.right: parent.right
        anchors.rightMargin: 18
        y: 134
        text: weather.condition
        color: Theme.textOnLightSecondary
        font.family: Theme.fontFamily
        font.pixelSize: 11
        elide: Text.ElideRight
    }

    Text {
        anchors.right: parent.right
        anchors.rightMargin: 18
        y: 116
        text: weather.city
        color: Theme.textOnLightSecondary
        font.family: Theme.fontFamily
        font.pixelSize: 10
    }
}
