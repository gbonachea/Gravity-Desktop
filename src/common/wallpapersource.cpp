#include "wallpapersource.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QUrl>

namespace
{

const QStringList &imageNameFilters()
{
    static const QStringList filters{
        QStringLiteral("*.jpg"),  QStringLiteral("*.jpeg"), QStringLiteral("*.png"),
        QStringLiteral("*.webp"), QStringLiteral("*.bmp"),  QStringLiteral("*.gif"),
        QStringLiteral("*.avif"), QStringLiteral("*.tif"),  QStringLiteral("*.tiff"),
        QStringLiteral("*.svg"),
    };
    return filters;
}

const QStringList &imageSuffixes()
{
    static const QStringList suffixes{
        QStringLiteral("jpg"),  QStringLiteral("jpeg"), QStringLiteral("png"),
        QStringLiteral("webp"), QStringLiteral("bmp"),  QStringLiteral("gif"),
        QStringLiteral("avif"), QStringLiteral("tif"),  QStringLiteral("tiff"),
        QStringLiteral("svg"),
    };
    return suffixes;
}

QString toLocalPath(QString value)
{
    value = value.trimmed();
    if (value.startsWith(QLatin1String("file://"))
        || value.startsWith(QLatin1String("file:"))) {
        return QUrl(value).toLocalFile();
    }
    return value;
}

bool isImageFile(const QString &path)
{
    const QFileInfo info(path);
    return info.isFile() && info.isReadable()
        && imageSuffixes().contains(info.suffix().toLower());
}

QString firstImageInDirectory(const QString &path)
{
    QDir directory(path);
    if (!directory.exists()) {
        return {};
    }

    const QStringList files =
        directory.entryList(imageNameFilters(), QDir::Files | QDir::Readable, QDir::Name);
    if (!files.isEmpty()) {
        return directory.absoluteFilePath(files.first());
    }

    const QStringList subdirectories =
        directory.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString &subdirectory : subdirectories) {
        const QString found = firstImageInDirectory(directory.filePath(subdirectory));
        if (!found.isEmpty()) {
            return found;
        }
    }

    return {};
}

QString firstImageInTree(const QString &root)
{
    if (!QFileInfo::exists(root)) {
        return {};
    }

    QDirIterator iterator(root, imageNameFilters(), QDir::Files | QDir::Readable,
                          QDirIterator::Subdirectories);
    if (iterator.hasNext()) {
        return iterator.next();
    }

    return {};
}

QString resolveCandidate(QString value)
{
    const QString path = toLocalPath(value);
    if (path.isEmpty()) {
        return {};
    }
    if (isImageFile(path)) {
        return path;
    }
    if (QFileInfo(path).isDir()) {
        return firstImageInDirectory(path);
    }
    return {};
}

QString valueFromLine(const QString &line, const QString &key)
{
    const QString trimmed = line.trimmed();
    if (!trimmed.startsWith(key + QLatin1Char('='))) {
        return {};
    }
    return trimmed.mid(key.size() + 1).trimmed();
}

QString scanConfigFile(const QString &path, const QStringList &keys)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }

    QString candidate;
    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine());
        for (const QString &key : keys) {
            const QString value = valueFromLine(line, key);
            if (value.isEmpty()) {
                continue;
            }

            if (key == QLatin1String("usersWallpapers")) {
                const QStringList entries = value.split(QLatin1Char(','), Qt::SkipEmptyParts);
                for (const QString &entry : entries) {
                    const QString found = resolveCandidate(entry);
                    if (!found.isEmpty()) {
                        candidate = found;
                    }
                }
            } else {
                const QString found = resolveCandidate(value);
                if (!found.isEmpty()) {
                    candidate = found;
                }
            }
        }
    }

    return candidate;
}

QString fromOverrideFile()
{
    const QString configDir =
        QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    const QString path = configDir + QStringLiteral("/stellarshell/wallpaper");

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }

    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine()).trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
            continue;
        }
        const QString found = resolveCandidate(line);
        if (!found.isEmpty()) {
            return found;
        }
    }

    return {};
}

QString fromKdeConfig()
{
    const QString configDir =
        QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);

    // Plasma stores the selected image wallpaper in the containment applet
    // configuration. Read the raw file so the nested [Containments][N] groups
    // are handled regardless of the QSettings backend.
    const QString applets =
        configDir + QStringLiteral("/plasma-org.kde.plasma.desktop-appletsrc");
    QString candidate = scanConfigFile(applets, {QStringLiteral("Image")});
    if (!candidate.isEmpty()) {
        return candidate;
    }

    // Plasma 6 also keeps a shared list of user wallpapers in plasmarc.
    const QString plasmarc = configDir + QStringLiteral("/plasmarc");
    return scanConfigFile(plasmarc, {QStringLiteral("usersWallpapers")});
}

QString fromCommonDirectories()
{
    const QString home = QDir::homePath();
    QStringList roots{
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
        home + QStringLiteral("/Imágenes"),
        home + QStringLiteral("/Imagens"),
        home + QStringLiteral("/Wallpapers"),
        home + QStringLiteral("/.local/share/wallpapers"),
        QStringLiteral("/usr/share/backgrounds"),
        QStringLiteral("/usr/share/wallpapers"),
    };
    roots.removeAll(QString());
    roots.removeDuplicates();

    for (const QString &root : roots) {
        const QString found = firstImageInTree(root);
        if (!found.isEmpty()) {
            return found;
        }
    }

    return {};
}

} // namespace

WallpaperSource::WallpaperSource(QObject *parent)
    : QObject(parent)
{
    reload();
}

void WallpaperSource::reload()
{
    QString path = qEnvironmentVariable("STELLAR_WALLPAPER");
    if (!path.isEmpty()) {
        path = resolveCandidate(path);
    }

    if (path.isEmpty()) {
        path = fromOverrideFile();
    }
    if (path.isEmpty()) {
        path = fromKdeConfig();
    }
    if (path.isEmpty()) {
        path = fromCommonDirectories();
    }

    setWallpaper(path);
}

void WallpaperSource::setWallpaper(const QString &path)
{
    if (path == m_path) {
        return;
    }

    m_path = path;
    if (m_path.isEmpty()) {
        m_source.clear();
        m_label = tr("Degradado Stellar");
    } else {
        m_source = QUrl::fromLocalFile(m_path).toString();
        m_label = QFileInfo(m_path).fileName();
    }

    emit sourceChanged();
}
