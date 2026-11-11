#pragma once

#include <QObject>
#include <QString>

/*!
 * Resolves the wallpaper used by StellarBackground.
 *
 * Lookup order:
 *   1. STELLAR_WALLPAPER environment variable.
 *   2. ~/.config/stellarshell/wallpaper (one path per line).
 *   3. The current KDE Plasma image wallpaper
 *      (~/.config/plasma-org.kde.plasma.desktop-appletsrc).
 *   4. The first entry of usersWallpapers in ~/.config/plasmarc.
 *   5. Common image directories (~/Pictures, ~/Imágenes, /usr/share/backgrounds,
 *      /usr/share/wallpapers) as a last resort.
 *
 * The QML side gets an already encoded file:// URL and only has to scale it.
 */
class WallpaperSource : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString source READ source NOTIFY sourceChanged)
    Q_PROPERTY(bool hasImage READ hasImage NOTIFY sourceChanged)
    Q_PROPERTY(QString label READ label NOTIFY sourceChanged)

public:
    explicit WallpaperSource(QObject *parent = nullptr);

    QString source() const { return m_source; }
    bool hasImage() const { return !m_source.isEmpty(); }
    QString label() const { return m_label; }

    Q_INVOKABLE void reload();

signals:
    void sourceChanged();

private:
    void setWallpaper(const QString &path);

    QString m_source;
    QString m_path;
    QString m_label;
};
