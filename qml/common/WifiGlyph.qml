import QtQuick

Canvas {
    id: root

    property bool online: true
    property color onColor: "#e6f3ff"
    property color offColor: "#ff9aa8"

    implicitWidth: 24
    implicitHeight: 24

    onOnlineChanged: requestPaint()
    onOnColorChanged: requestPaint()
    onOffColorChanged: requestPaint()

    onPaint: {
        const ctx = getContext("2d")
        ctx.clearRect(0, 0, width, height)

        const color = online ? onColor : offColor
        const cx = width / 2
        const cy = height * 0.74
        const lineWidth = Math.max(1.6, width * 0.075)

        ctx.strokeStyle = color
        ctx.fillStyle = color
        ctx.lineWidth = lineWidth
        ctx.lineCap = "round"

        ctx.beginPath()
        ctx.arc(cx, cy, width * 0.075, 0, Math.PI * 2)
        ctx.fill()

        for (let i = 1; i <= 3; ++i) {
            const radius = width * (0.11 + i * 0.115)
            ctx.beginPath()
            ctx.arc(cx, cy, radius, Math.PI * 1.23, Math.PI * 1.77)
            ctx.stroke()
        }

        if (!online) {
            ctx.strokeStyle = offColor
            ctx.lineWidth = Math.max(1.8, width * 0.08)
            ctx.beginPath()
            ctx.moveTo(width * 0.2, height * 0.2)
            ctx.lineTo(width * 0.8, height * 0.8)
            ctx.stroke()
        }
    }
}
