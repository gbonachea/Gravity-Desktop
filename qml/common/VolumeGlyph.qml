import QtQuick

Canvas {
    id: root

    property int volume: -1
    property color glyphColor: "#e6f3ff"

    implicitWidth: 24
    implicitHeight: 24

    onVolumeChanged: requestPaint()
    onGlyphColorChanged: requestPaint()

    onPaint: {
        const ctx = getContext("2d")
        ctx.clearRect(0, 0, width, height)

        const color = volume < 0 ? "#8fb5e6" : glyphColor
        ctx.strokeStyle = color
        ctx.fillStyle = color
        ctx.lineWidth = Math.max(1.5, width * 0.07)
        ctx.lineCap = "round"

        const cy = height / 2
        ctx.beginPath()
        ctx.moveTo(width * 0.12, height * 0.40)
        ctx.lineTo(width * 0.28, height * 0.40)
        ctx.lineTo(width * 0.46, height * 0.24)
        ctx.lineTo(width * 0.46, height * 0.76)
        ctx.lineTo(width * 0.28, height * 0.60)
        ctx.lineTo(width * 0.12, height * 0.60)
        ctx.closePath()
        ctx.fill()

        const level = volume < 0 ? 2 : (volume === 0 ? 0 : (volume < 40 ? 1 : (volume < 75 ? 2 : 3)))
        for (let i = 1; i <= level; ++i) {
            ctx.beginPath()
            ctx.arc(width * 0.48, cy, width * (0.06 + i * 0.095), -Math.PI * 0.42, Math.PI * 0.42)
            ctx.stroke()
        }

        if (volume === 0) {
            ctx.strokeStyle = "#ff9aa8"
            ctx.lineWidth = Math.max(1.6, width * 0.075)
            ctx.beginPath()
            ctx.moveTo(width * 0.66, height * 0.34)
            ctx.lineTo(width * 0.86, height * 0.66)
            ctx.moveTo(width * 0.86, height * 0.34)
            ctx.lineTo(width * 0.66, height * 0.66)
            ctx.stroke()
        }
    }
}
