#pragma once

#include <QQuickImageProvider>

/*!
 * Resolves freedesktop icon names (and absolute file paths) through
 * QIcon::fromTheme and caches the resulting pixmaps.
 */
class IconImageProvider : public QQuickImageProvider
{
public:
    IconImageProvider();

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    QImage m_fallback;
};
