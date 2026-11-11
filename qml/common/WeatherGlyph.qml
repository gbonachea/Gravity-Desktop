import QtQuick

Canvas {
    id: root

    property string kind: "partly"

    implicitWidth: 58
    implicitHeight: 58

    onKindChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()

    function drawSun(ctx, cx, cy, radius) {
        ctx.save()
        ctx.fillStyle = "#ffd54f"
        ctx.strokeStyle = "#ffcf3f"
        ctx.lineWidth = Math.max(1.8, root.width * 0.035)
        ctx.lineCap = "round"

        for (let i = 0; i < 8; ++i) {
            const angle = (Math.PI * 2 * i) / 8
            ctx.beginPath()
            ctx.moveTo(cx + Math.cos(angle) * radius * 1.35,
                       cy + Math.sin(angle) * radius * 1.35)
            ctx.lineTo(cx + Math.cos(angle) * radius * 1.75,
                       cy + Math.sin(angle) * radius * 1.75)
            ctx.stroke()
        }

        ctx.beginPath()
        ctx.arc(cx, cy, radius, 0, Math.PI * 2)
        ctx.fill()
        ctx.restore()
    }

    function drawCloud(ctx, x, y, scale) {
        ctx.save()
        ctx.fillStyle = "#f4fbff"
        ctx.strokeStyle = "#bfe3ff"
        ctx.lineWidth = Math.max(1.2, root.width * 0.02)

        ctx.beginPath()
        ctx.arc(x + 0.26 * scale, y + 0.44 * scale, 0.20 * scale, Math.PI * 0.8, Math.PI * 1.9)
        ctx.arc(x + 0.50 * scale, y + 0.34 * scale, 0.26 * scale, Math.PI, Math.PI * 1.9)
        ctx.arc(x + 0.74 * scale, y + 0.46 * scale, 0.18 * scale, Math.PI * 1.1, Math.PI * 2.1)
        ctx.lineTo(x + 0.80 * scale, y + 0.62 * scale)
        ctx.lineTo(x + 0.18 * scale, y + 0.62 * scale)
        ctx.closePath()
        ctx.fill()
        ctx.stroke()
        ctx.restore()
    }

    function drawRain(ctx, x, y, scale) {
        ctx.save()
        ctx.strokeStyle = "#62b8ff"
        ctx.lineWidth = Math.max(1.6, root.width * 0.028)
        ctx.lineCap = "round"
        for (let i = 0; i < 3; ++i) {
            ctx.beginPath()
            ctx.moveTo(x + (0.28 + i * 0.16) * scale, y + 0.66 * scale)
            ctx.lineTo(x + (0.23 + i * 0.16) * scale, y + 0.82 * scale)
            ctx.stroke()
        }
        ctx.restore()
    }

    onPaint: {
        const ctx = getContext("2d")
        ctx.clearRect(0, 0, width, height)

        const s = Math.min(width, height)
        const sunX = width * 0.40
        const sunY = height * 0.36

        if (kind === "sun") {
            drawSun(ctx, width / 2, height / 2, s * 0.20)
        } else if (kind === "partly") {
            drawSun(ctx, sunX, sunY, s * 0.15)
            drawCloud(ctx, width * 0.26, height * 0.37, s * 0.70)
        } else if (kind === "cloud") {
            drawCloud(ctx, width * 0.16, height * 0.26, s * 0.80)
        } else if (kind === "fog") {
            drawCloud(ctx, width * 0.16, height * 0.20, s * 0.72)
            ctx.save()
            ctx.strokeStyle = "#b9d8ef"
            ctx.lineWidth = Math.max(1.4, s * 0.025)
            ctx.lineCap = "round"
            for (let i = 0; i < 3; ++i) {
                ctx.beginPath()
                ctx.moveTo(width * 0.22, height * (0.68 + i * 0.08))
                ctx.lineTo(width * 0.78, height * (0.68 + i * 0.08))
                ctx.stroke()
            }
            ctx.restore()
        } else if (kind === "rain") {
            drawCloud(ctx, width * 0.14, height * 0.18, s * 0.74)
            drawRain(ctx, width * 0.14, height * 0.18, s * 0.74)
        } else if (kind === "snow") {
            drawCloud(ctx, width * 0.14, height * 0.18, s * 0.74)
            ctx.save()
            ctx.fillStyle = "#e8f7ff"
            for (let i = 0; i < 3; ++i) {
                ctx.beginPath()
                ctx.arc(width * (0.34 + i * 0.15), height * 0.76, s * 0.035, 0, Math.PI * 2)
                ctx.fill()
            }
            ctx.restore()
        } else if (kind === "storm") {
            drawCloud(ctx, width * 0.14, height * 0.16, s * 0.74)
            ctx.save()
            ctx.fillStyle = "#ffd54f"
            ctx.beginPath()
            ctx.moveTo(width * 0.52, height * 0.58)
            ctx.lineTo(width * 0.42, height * 0.78)
            ctx.lineTo(width * 0.52, height * 0.78)
            ctx.lineTo(width * 0.46, height * 0.94)
            ctx.lineTo(width * 0.64, height * 0.69)
            ctx.lineTo(width * 0.54, height * 0.69)
            ctx.closePath()
            ctx.fill()
            ctx.restore()
        }
    }
}
