#include "iconimageprovider.h"

#include <QFileInfo>
#include <QIcon>
#include <QPainter>
#include <QPainterPath>
#include <QUrl>

namespace
{

QImage makeFallbackIcon(int size)
{
    QImage image(size, size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF tile(1.0, 1.0, size - 2.0, size - 2.0);
    QLinearGradient gradient(tile.topLeft(), tile.bottomRight());
    gradient.setColorAt(0.0, QColor(0x5d, 0xa9, 0xff));
    gradient.setColorAt(1.0, QColor(0x1f, 0x5f, 0xc9));

    QPainterPath rounded;
    rounded.addRoundedRect(tile, size * 0.24, size * 0.24);
    painter.fillPath(rounded, gradient);

    painter.setPen(QPen(QColor(255, 255, 255, 235), qMax(1.5, size * 0.055)));
    painter.setBrush(Qt::NoBrush);
    const QRectF windowRect(size * 0.26, size * 0.30, size * 0.48, size * 0.40);
    painter.drawRoundedRect(windowRect, size * 0.05, size * 0.05);
    painter.drawLine(QPointF(windowRect.left(), size * 0.43),
                     QPointF(windowRect.right(), size * 0.43));
    painter.end();

    return image;
}

} // namespace

IconImageProvider::IconImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
    m_fallback = makeFallbackIcon(128);
}

QImage IconImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    // El QML añade ?theme=... para invalidar la caché al cambiar de tema.
    QString rawId = id;
    const int query = rawId.indexOf(QLatin1Char('?'));
    if (query >= 0) {
        rawId = rawId.left(query);
    }
    QString key = QUrl::fromPercentEncoding(rawId.toUtf8());
    if (key.isEmpty()) {
        key = QStringLiteral("application-x-executable");
    }

    int pixelSize = 64;
    if (requestedSize.isValid()) {
        pixelSize = qBound(8, qMax(requestedSize.width(), requestedSize.height()), 512);
    }

    QIcon icon;
    if (QFileInfo(key).isAbsolute() && QFileInfo::exists(key)) {
        icon = QIcon(key);
    } else {
        icon = QIcon::fromTheme(key);
        if (icon.isNull()) {
            // Some applications expose reverse-DNS app ids. Try the last
            // component as an icon name as a last resort.
            const QString lastPart = key.section(QLatin1Char('.'), -1);
            if (!lastPart.isEmpty()) {
                icon = QIcon::fromTheme(lastPart);
            }
        }
    }

    QImage result;
    if (!icon.isNull()) {
        const QPixmap pixmap = icon.pixmap(QSize(pixelSize, pixelSize), 1.0);
        if (!pixmap.isNull()) {
            result = pixmap.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
        }
    }

    if (result.isNull()) {
        result = m_fallback.scaled(pixelSize,
                                   pixelSize,
                                   Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation);
    }

    if (size) {
        *size = result.size();
    }
    return result;
}
