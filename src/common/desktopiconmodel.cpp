#include "desktopiconmodel.h"

#include "applauncher.h"
#include "desktopentrymodel.h"
#include "filemanager.h"
#include "launchenvironment.h"
#include "settingscontroller.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QMimeDatabase>
#include <QProcess>
#include <QStandardPaths>

namespace
{

const QStringList &reservedDesktopFiles()
{
    static const QStringList files = {
        QStringLiteral("stellar-equipo.desktop"),
        QStringLiteral("stellar-usuario.desktop"),
        QStringLiteral("stellar-papelera.desktop"),
    };
    return files;
}

QString terminalProgram()
{
    static const QStringList candidates = {
        QStringLiteral("x-terminal-emulator"),
        QStringLiteral("kgx"),
        QStringLiteral("gnome-terminal"),
        QStringLiteral("konsole"),
        QStringLiteral("alacritty"),
        QStringLiteral("kitty"),
        QStringLiteral("foot"),
        QStringLiteral("xterm"),
    };
    for (const QString &candidate : candidates) {
        const QString path = QStandardPaths::findExecutable(candidate);
        if (!path.isEmpty()) {
            return path;
        }
    }
    return {};
}

} // namespace

DesktopIconModel::DesktopIconModel(SettingsController *settings, QObject *parent)
    : QAbstractListModel(parent)
    , m_settings(settings)
{
    m_reloadTimer.setSingleShot(true);
    m_reloadTimer.setInterval(150);
    connect(&m_reloadTimer, &QTimer::timeout, this, &DesktopIconModel::rebuild);

    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this,
            &DesktopIconModel::scheduleReload);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this,
            &DesktopIconModel::scheduleReload);
    if (m_settings) {
        connect(m_settings, &SettingsController::desktopIconsChanged, this,
                &DesktopIconModel::scheduleReload);
    }

    rebuild();
}

int DesktopIconModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_items.size());
}

QVariant DesktopIconModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return {};
    }
    const Item &item = m_items.at(index.row());
    switch (role) {
    case NameRole:
        return item.name;
    case IconRole:
        return item.icon;
    case KindRole:
        return item.kind;
    case PathRole:
        return item.path;
    case RemovableRole:
        return item.removable;
    case RemoveKindRole:
        return item.removeKind;
    default:
        return {};
    }
}

QHash<int, QByteArray> DesktopIconModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {IconRole, "icon"},
        {KindRole, "kind"},
        {PathRole, "path"},
        {RemovableRole, "removable"},
        {RemoveKindRole, "removeKind"},
    };
}

QVariantMap DesktopIconModel::itemAt(int row) const
{
    if (row < 0 || row >= m_items.size()) {
        return {};
    }
    const Item &item = m_items.at(row);
    return {
        {QStringLiteral("name"), item.name},
        {QStringLiteral("icon"), item.icon},
        {QStringLiteral("kind"), item.kind},
        {QStringLiteral("path"), item.path},
        {QStringLiteral("removable"), item.removable},
        {QStringLiteral("removeKind"), item.removeKind},
    };
}

void DesktopIconModel::reload()
{
    rebuild();
}

void DesktopIconModel::scheduleReload()
{
    m_reloadTimer.start();
}

QString DesktopIconModel::desktopDir() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    if (dir.isEmpty()) {
        dir = QDir::homePath() + QStringLiteral("/Desktop");
    }
    QDir().mkpath(dir);
    return dir;
}

void DesktopIconModel::rebuild()
{
    const QString dirPath = desktopDir();
    if (!m_watcher.directories().contains(dirPath)) {
        m_watcher.addPath(dirPath);
    }

    // Los interruptores del Centro de configuración viven en otro proceso:
    // recarga el archivo y vuelve a vigilarlo por si fue reemplazado.
    if (m_settings) {
        m_settings->reloadSettings();
        const QString settingsFile = m_settings->settingsFilePath();
        if (!settingsFile.isEmpty() && QFileInfo::exists(settingsFile)
            && !m_watcher.files().contains(settingsFile)) {
            m_watcher.addPath(settingsFile);
        }
    }

    beginResetModel();
    m_items.clear();

    if (m_settings) {
        if (m_settings->desktopShowComputer()) {
            m_items.append({tr("Equipo"), QStringLiteral("computer"),
                            QStringLiteral("special"), QStringLiteral("computer"),
                            QStringLiteral("computer"), true});
        }
        if (m_settings->desktopShowUser()) {
            m_items.append({tr("Usuario"), QStringLiteral("user-home"),
                            QStringLiteral("special"), QDir::homePath(),
                            QStringLiteral("user"), true});
        }
        if (m_settings->desktopShowTrash()) {
            m_items.append({tr("Papelera"), QStringLiteral("user-trash"),
                            QStringLiteral("special"), QStringLiteral("trash"),
                            QStringLiteral("trash"), true});
        }
    }

    const QDir desktop(dirPath);
    const QFileInfoList entries =
        desktop.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Readable,
                              QDir::DirsFirst | QDir::Name | QDir::IgnoreCase);

    QMimeDatabase mimeDatabase;
    for (const QFileInfo &info : entries) {
        if (reservedDesktopFiles().contains(info.fileName())) {
            continue;
        }

        Item item;
        item.path = info.absoluteFilePath();
        item.removable = true;

        if (info.isDir()) {
            item.name = info.fileName();
            item.icon = QStringLiteral("folder");
            item.kind = QStringLiteral("folder");
            item.removeKind = QStringLiteral("trashfile");
        } else if (info.suffix().compare(QLatin1String("desktop"), Qt::CaseInsensitive) == 0) {
            const DesktopEntry entry = parseDesktopFile(item.path);
            if (!isValidApplication(entry)) {
                continue;
            }
            item.name = entry.name;
            item.icon = entry.icon.isEmpty()
                ? QStringLiteral("application-x-executable")
                : entry.icon;
            item.kind = QStringLiteral("application");
            item.removeKind = QStringLiteral("shortcut");
        } else {
            item.name = info.fileName();
            const QString mimeIcon = mimeDatabase.mimeTypeForFile(info).iconName();
            item.icon = mimeIcon.isEmpty() ? QStringLiteral("text-x-generic") : mimeIcon;
            item.kind = QStringLiteral("file");
            item.removeKind = QStringLiteral("trashfile");
        }

        m_items.append(item);
    }

    endResetModel();
    emit countChanged();
}

bool DesktopIconModel::open(int row)
{
    if (row < 0 || row >= m_items.size()) {
        return false;
    }
    const Item &item = m_items.at(row);

    if (item.kind == QLatin1String("special")) {
        QString location = item.path;
        if (location == QLatin1String("computer")) {
            location = QStringLiteral("/");
        } else if (location == QLatin1String("trash")) {
            location = QStringLiteral("trash");
        }

        const bool ok = Stellar::openWithFileManager(location);
        if (!ok) {
            emit actionFailed(tr("No se pudo abrir %1 con el gestor de archivos")
                                  .arg(item.name));
        }
        return ok;
    }

    if (item.kind == QLatin1String("application")) {
        AppLauncher launcher;
        connect(&launcher, &AppLauncher::launchFailed, this, &DesktopIconModel::actionFailed);
        if (launcher.launchDesktopFile(item.path)) {
            return true;
        }

        // Respaldo: gio conoce los lanzadores .desktop incluso cuando el
        // índice XDG del shell no los tiene registrados.
        const QString gio = QStandardPaths::findExecutable(QStringLiteral("gio"));
        if (!gio.isEmpty() && Stellar::startDetached(gio,
                                                     {QStringLiteral("launch"), item.path},
                                                     QDir::homePath())) {
            return true;
        }
        return false;
    }

    const bool ok = Stellar::openFileOrFolder(item.path);
    if (!ok) {
        emit actionFailed(tr("No se pudo abrir %1").arg(item.name));
    }
    return ok;
}

bool DesktopIconModel::openTerminal(int row)
{
    QString workingDirectory = desktopDir();
    if (row >= 0 && row < m_items.size()) {
        const Item &item = m_items.at(row);
        const QFileInfo info(item.path);
        if (info.isDir()) {
            workingDirectory = info.absoluteFilePath();
        } else if (info.exists()) {
            workingDirectory = info.absolutePath();
        }
    }

    const QString terminal = terminalProgram();
    if (terminal.isEmpty()) {
        emit actionFailed(tr("No se encontró un emulador de terminal"));
        return false;
    }
    const bool ok = Stellar::startDetached(terminal, {}, workingDirectory);
    if (!ok) {
        emit actionFailed(tr("No se pudo abrir el terminal"));
    }
    return ok;
}

bool DesktopIconModel::removeFile(const QString &path)
{
    const QFileInfo info(path);
    if (!info.exists()) {
        return false;
    }

    // Los archivos y carpetas del usuario se envían a la papelera cuando gio
    // está disponible; los accesos .desktop se eliminan directamente.
    if (info.isDir() || info.suffix().compare(QLatin1String("desktop"), Qt::CaseInsensitive) != 0) {
        const QString gio = QStandardPaths::findExecutable(QStringLiteral("gio"));
        if (!gio.isEmpty() && Stellar::startDetached(gio, {QStringLiteral("trash"), path})) {
            return true;
        }
    }

    if (info.isDir()) {
        return QDir(path).removeRecursively();
    }
    return QFile::remove(path);
}

bool DesktopIconModel::remove(int row)
{
    if (row < 0 || row >= m_items.size()) {
        return false;
    }
    const Item item = m_items.at(row);
    bool ok = false;

    if (item.removeKind == QLatin1String("computer")) {
        if (m_settings) {
            m_settings->setDesktopShowComputer(false);
            ok = true;
        }
    } else if (item.removeKind == QLatin1String("user")) {
        if (m_settings) {
            m_settings->setDesktopShowUser(false);
            ok = true;
        }
    } else if (item.removeKind == QLatin1String("trash")) {
        if (m_settings) {
            m_settings->setDesktopShowTrash(false);
            ok = true;
        }
    } else {
        ok = removeFile(item.path);
    }

    if (!ok) {
        emit actionFailed(tr("No se pudo quitar %1 del escritorio").arg(item.name));
        return false;
    }
    reload();
    return true;
}

bool DesktopIconModel::createFolder()
{
    const QDir desktop(desktopDir());
    QString name = tr("Nueva carpeta");
    int suffix = 2;
    while (desktop.exists(name)) {
        name = tr("Nueva carpeta (%1)").arg(suffix++);
    }
    if (!desktop.mkdir(name)) {
        emit actionFailed(tr("No se pudo crear la carpeta"));
        return false;
    }
    reload();
    return true;
}

bool DesktopIconModel::openDesktopFolder()
{
    const bool ok = Stellar::openWithFileManager(desktopDir());
    if (!ok) {
        emit actionFailed(tr("No se pudo abrir la carpeta del escritorio"));
    }
    return ok;
}
