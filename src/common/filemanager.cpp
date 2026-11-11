#include "filemanager.h"

#include "desktopentrymodel.h"
#include "launchenvironment.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QUrl>

namespace
{

QString runAndTrim(const QString &program, const QStringList &arguments)
{
    QProcess process;
    process.start(program, arguments);
    if (!process.waitForFinished(2000)) {
        process.kill();
        process.waitForFinished(500);
        return {};
    }
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        return {};
    }
    return QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
}

QStringList splitExecLine(const QString &commandLine)
{
    QStringList tokens;
    QString current;
    bool inSingle = false;
    bool inDouble = false;
    bool escaped = false;

    for (const QChar c : commandLine) {
        if (escaped) {
            current.append(c);
            escaped = false;
            continue;
        }
        if (c == QLatin1Char('\\') && !inSingle) {
            escaped = true;
            continue;
        }
        if (c == QLatin1Char('"') && !inSingle) {
            inDouble = !inDouble;
            continue;
        }
        if (c == QLatin1Char('\'') && !inDouble) {
            inSingle = !inSingle;
            continue;
        }
        if (c.isSpace() && !inSingle && !inDouble) {
            if (!current.isEmpty()) {
                tokens.append(current);
                current.clear();
            }
            continue;
        }
        current.append(c);
    }
    if (!current.isEmpty()) {
        tokens.append(current);
    }
    return tokens;
}

QString executableFromDesktopFile(const QString &desktopFile)
{
    if (desktopFile.isEmpty()) {
        return {};
    }

    const DesktopEntry *indexed = DesktopEntryIndex::instance().findByDesktopFile(desktopFile);
    DesktopEntry entry;
    if (indexed) {
        entry = *indexed;
    } else {
        const QStringList roots = QStandardPaths::standardLocations(
            QStandardPaths::ApplicationsLocation);
        for (const QString &root : roots) {
            const QString path = root + QLatin1Char('/') + desktopFile;
            if (QFileInfo::exists(path)) {
                entry = parseDesktopFile(path);
                break;
            }
        }
        if (entry.exec.isEmpty()) {
            const QString path = desktopFile;
            if (QFileInfo(path).isFile()) {
                entry = parseDesktopFile(path);
            }
        }
    }

    if (entry.exec.isEmpty()) {
        return {};
    }

    const QStringList tokens = splitExecLine(entry.exec);
    if (tokens.isEmpty()) {
        return {};
    }
    QString program = tokens.first();
    if (program.startsWith(QLatin1Char('%'))) {
        return {};
    }
    if (QFileInfo(program).isAbsolute()) {
        return QFileInfo(program).isExecutable() ? program : QString();
    }
    return QStandardPaths::findExecutable(program);
}

QString normalizeLocation(const QString &location)
{
    if (location.isEmpty() || location == QLatin1String("computer")) {
        return QStringLiteral("/");
    }
    if (location == QLatin1String("trash")) {
        const QString trash = QDir::homePath()
            + QStringLiteral("/.local/share/Trash/files");
        return QFileInfo::exists(trash) ? trash : QStringLiteral("trash:///");
    }
    if (location.startsWith(QLatin1String("file://"))) {
        return QUrl(location).toLocalFile();
    }
    return location;
}

bool launch(const QString &program, const QStringList &arguments)
{
    if (program.isEmpty()) {
        return false;
    }
    return Stellar::startDetached(program, arguments, QDir::homePath());
}

} // namespace

namespace Stellar
{

QString defaultFileManagerExecutable()
{
    const QString xdgMime = QStandardPaths::findExecutable(QStringLiteral("xdg-mime"));
    if (!xdgMime.isEmpty()) {
        const QString desktop = runAndTrim(
            xdgMime, {QStringLiteral("query"), QStringLiteral("default"),
                      QStringLiteral("inode/directory")});
        const QString executable = executableFromDesktopFile(desktop);
        if (!executable.isEmpty()) {
            return executable;
        }
    }

    const QStringList candidates = {
        QStringLiteral("dolphin"),
        QStringLiteral("nautilus"),
        QStringLiteral("thunar"),
        QStringLiteral("pcmanfm-qt"),
        QStringLiteral("pcmanfm"),
        QStringLiteral("nemo"),
        QStringLiteral("caja"),
        QStringLiteral("dde-file-manager"),
        QStringLiteral("nautilus-autorun"),
    };
    for (const QString &candidate : candidates) {
        const QString path = QStandardPaths::findExecutable(candidate);
        if (!path.isEmpty()) {
            return path;
        }
    }
    return {};
}

QString defaultFileManagerName()
{
    const QString path = defaultFileManagerExecutable();
    return path.isEmpty() ? QString() : QFileInfo(path).fileName();
}

bool openWithFileManager(const QString &location)
{
    const QString target = normalizeLocation(location);
    const QString manager = defaultFileManagerExecutable();

    if (!manager.isEmpty() && launch(manager, {target})) {
        return true;
    }

    const QString gio = QStandardPaths::findExecutable(QStringLiteral("gio"));
    if (!gio.isEmpty() && launch(gio, {QStringLiteral("open"), target})) {
        return true;
    }

    const QString xdgOpen = QStandardPaths::findExecutable(QStringLiteral("xdg-open"));
    if (!xdgOpen.isEmpty() && launch(xdgOpen, {target})) {
        return true;
    }

    return false;
}

bool openFileOrFolder(const QString &path)
{
    const QFileInfo info(path);
    if (info.isDir()) {
        return openWithFileManager(path);
    }

    if (QDesktopServices::openUrl(QUrl::fromLocalFile(path))) {
        return true;
    }

    const QString gio = QStandardPaths::findExecutable(QStringLiteral("gio"));
    if (!gio.isEmpty() && launch(gio, {QStringLiteral("open"), path})) {
        return true;
    }

    // Mostrar al menos la carpeta que contiene el archivo.
    return openWithFileManager(info.absolutePath());
}

} // namespace Stellar
