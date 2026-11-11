#include "defaultappscontroller.h"

#include "desktopentrymodel.h"
#include "launchenvironment.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QSet>
#include <QStandardPaths>

namespace
{

bool run(const QString &program, const QStringList &arguments, QString *output = nullptr)
{
    QProcess process;
    process.start(program, arguments);
    if (!process.waitForFinished(3000)) {
        process.kill();
        process.waitForFinished(500);
        return false;
    }
    if (output) {
        *output = QString::fromLocal8Bit(process.readAllStandardOutput());
    }
    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

QString desktopFileFromName(const QString &value)
{
    const QString trimmed = value.trimmed();
    if (trimmed.endsWith(QLatin1String(".desktop"))) {
        return QFileInfo(trimmed).fileName();
    }
    return trimmed;
}

} // namespace

DefaultAppsController::DefaultAppsController(QObject *parent)
    : QObject(parent)
{
}

QVector<DefaultAppsController::Category> DefaultAppsController::definitions() const
{
    return {
        {QStringLiteral("web"), tr("Navegador web"),
         {QStringLiteral("x-scheme-handler/http"), QStringLiteral("x-scheme-handler/https"),
          QStringLiteral("text/html")},
         {QStringLiteral("WebBrowser"), QStringLiteral("Network")}, true},
        {QStringLiteral("mail"), tr("Cliente de correo"),
         {QStringLiteral("x-scheme-handler/mailto"), QStringLiteral("message/rfc822")},
         {QStringLiteral("Email"), QStringLiteral("Network")}, false},
        {QStringLiteral("files"), tr("Gestor de archivos"),
         {QStringLiteral("inode/directory")},
         {QStringLiteral("FileManager")}, false},
        {QStringLiteral("text"), tr("Editor de texto"),
         {QStringLiteral("text/plain")},
         {QStringLiteral("TextEditor"), QStringLiteral("Utility")}, false},
        {QStringLiteral("image"), tr("Visor de imágenes"),
         {QStringLiteral("image/png"), QStringLiteral("image/jpeg")},
         {QStringLiteral("Graphics"), QStringLiteral("Viewer"),
          QStringLiteral("Photography")}, false},
        {QStringLiteral("video"), tr("Reproductor de vídeo"),
         {QStringLiteral("video/mp4"), QStringLiteral("video/x-matroska"),
          QStringLiteral("video/webm")},
         {QStringLiteral("AudioVideo"), QStringLiteral("Video"),
          QStringLiteral("Player")}, false},
        {QStringLiteral("music"), tr("Reproductor de música"),
         {QStringLiteral("audio/mpeg"), QStringLiteral("audio/flac"),
          QStringLiteral("audio/ogg")},
         {QStringLiteral("Audio"), QStringLiteral("Music"),
          QStringLiteral("Player")}, false},
        {QStringLiteral("pdf"), tr("Visor de PDF"),
         {QStringLiteral("application/pdf")},
         {QStringLiteral("Office"), QStringLiteral("Viewer")}, false},
    };
}

QVariantList DefaultAppsController::categories() const
{
    QVariantList result;

    for (const Category &category : definitions()) {
        const QString current = queryDefault(category);
        QVariantList choices = choicesFor(category);

        // El actual puede no aparecer entre las recomendaciones del MIME.
        bool found = false;
        for (const QVariant &choice : choices) {
            if (choice.toMap().value(QStringLiteral("file")).toString() == current) {
                found = true;
                break;
            }
        }
        if (!found && !current.isEmpty()) {
            choices.prepend(QVariantMap{
                {QStringLiteral("file"), current},
                {QStringLiteral("name"), appName(current)},
                {QStringLiteral("icon"), appIcon(current)},
            });
        }

        result.append(QVariantMap{
            {QStringLiteral("id"), category.id},
            {QStringLiteral("label"), category.label},
            {QStringLiteral("current"), current},
            {QStringLiteral("currentName"),
             current.isEmpty() ? tr("Sin asignar") : appName(current)},
            {QStringLiteral("currentIcon"), appIcon(current)},
            {QStringLiteral("choices"), choices},
        });
    }

    return result;
}

QString DefaultAppsController::queryDefault(const Category &category) const
{
    const QString xdgMime = QStandardPaths::findExecutable(QStringLiteral("xdg-mime"));
    if (!xdgMime.isEmpty() && !category.mimeTypes.isEmpty()) {
        QString output;
        if (run(xdgMime, {QStringLiteral("query"), QStringLiteral("default"),
                          category.mimeTypes.first()}, &output)) {
            const QString file = desktopFileFromName(output);
            if (!file.isEmpty()) {
                return file;
            }
        }
    }

    const QString gio = QStandardPaths::findExecutable(QStringLiteral("gio"));
    if (!gio.isEmpty() && !category.mimeTypes.isEmpty()) {
        QString output;
        if (run(gio, {QStringLiteral("mime"), category.mimeTypes.first()}, &output)) {
            for (const QString &line : output.split(QLatin1Char('\n'))) {
                if (line.contains(QLatin1String(".desktop"))) {
                    const int colon = line.indexOf(QLatin1Char(':'));
                    const QString value =
                        colon >= 0 ? line.mid(colon + 1).trimmed() : line.trimmed();
                    const QString file = desktopFileFromName(value);
                    if (!file.isEmpty()) {
                        return file;
                    }
                }
            }
        }
    }

    return {};
}

QVariantList DefaultAppsController::choicesFor(const Category &category) const
{
    QSet<QString> files;
    QVariantList choices;

    const QString gio = QStandardPaths::findExecutable(QStringLiteral("gio"));
    if (!gio.isEmpty()) {
        for (const QString &mime : category.mimeTypes) {
            QString output;
            if (!run(gio, {QStringLiteral("mime"), mime}, &output)) {
                continue;
            }
            for (const QString &line : output.split(QLatin1Char('\n'))) {
                const QString trimmed = line.trimmed();
                if (!trimmed.endsWith(QLatin1String(".desktop"))) {
                    continue;
                }
                const QString file = desktopFileFromName(trimmed);
                if (!file.isEmpty() && !files.contains(file)) {
                    files.insert(file);
                    choices.append(QVariantMap{
                        {QStringLiteral("file"), file},
                        {QStringLiteral("name"), appName(file)},
                        {QStringLiteral("icon"), appIcon(file)},
                    });
                }
            }
        }
    }

    if (choices.isEmpty()) {
        // Respaldo: aplicaciones del índice que pertenecen a las categorías
        // Freedesktop típicas de este tipo de contenido.
        const auto &entries = DesktopEntryIndex::instance().entries();
        for (const DesktopEntry &entry : entries) {
            bool matches = false;
            for (const QString &wanted : category.appCategories) {
                if (entry.categories.contains(wanted)) {
                    matches = true;
                    break;
                }
            }
            if (!matches || files.contains(entry.fileName)) {
                continue;
            }
            files.insert(entry.fileName);
            choices.append(QVariantMap{
                {QStringLiteral("file"), entry.fileName},
                {QStringLiteral("name"), entry.name},
                {QStringLiteral("icon"), entry.icon.isEmpty()
                     ? QStringLiteral("application-x-executable")
                     : entry.icon},
            });
        }
    }

    return choices;
}

bool DefaultAppsController::setDefault(const QString &categoryId,
                                       const QString &desktopFile)
{
    if (categoryId.isEmpty() || desktopFile.isEmpty()) {
        return false;
    }

    Category selected;
    bool found = false;
    for (const Category &category : definitions()) {
        if (category.id == categoryId) {
            selected = category;
            found = true;
            break;
        }
    }
    if (!found) {
        return false;
    }

    bool any = false;
    for (const QString &mime : selected.mimeTypes) {
        if (setMimeDefault(desktopFile, mime)) {
            any = true;
        }
    }

    if (selected.webBrowser) {
        const QString xdgSettings =
            QStandardPaths::findExecutable(QStringLiteral("xdg-settings"));
        if (!xdgSettings.isEmpty()) {
            run(xdgSettings, {QStringLiteral("set"),
                              QStringLiteral("default-web-browser"), desktopFile});
        }
    }

    if (!any) {
        emit errorOccurred(tr("No se pudo guardar la aplicación predeterminada"));
    }

    emit categoriesChanged();
    return any;
}

bool DefaultAppsController::setMimeDefault(const QString &desktopFile,
                                           const QString &mimeType)
{
    const QString xdgMime = QStandardPaths::findExecutable(QStringLiteral("xdg-mime"));
    if (!xdgMime.isEmpty()) {
        if (run(xdgMime, {QStringLiteral("default"), desktopFile, mimeType})) {
            return true;
        }
    }

    const QString gio = QStandardPaths::findExecutable(QStringLiteral("gio"));
    if (!gio.isEmpty()) {
        return run(gio, {QStringLiteral("mime"), mimeType, desktopFile});
    }
    return false;
}

QString DefaultAppsController::appName(const QString &desktopFile)
{
    if (desktopFile.isEmpty()) {
        return {};
    }
    if (const DesktopEntry *entry =
            DesktopEntryIndex::instance().findByDesktopFile(desktopFile)) {
        return entry->name;
    }
    const QString path = QStringLiteral("/usr/share/applications/") + desktopFile;
    if (QFileInfo(path).isFile()) {
        const DesktopEntry parsed = parseDesktopFile(path);
        if (!parsed.name.isEmpty()) {
            return parsed.name;
        }
    }
    QString name = QFileInfo(desktopFile).completeBaseName();
    name.replace(QLatin1Char('-'), QLatin1Char(' '));
    return name;
}

QString DefaultAppsController::appIcon(const QString &desktopFile)
{
    if (desktopFile.isEmpty()) {
        return QStringLiteral("application-x-executable");
    }
    if (const DesktopEntry *entry =
            DesktopEntryIndex::instance().findByDesktopFile(desktopFile)) {
        return entry->icon.isEmpty() ? QStringLiteral("application-x-executable")
                                     : entry->icon;
    }
    const QString path = QStringLiteral("/usr/share/applications/") + desktopFile;
    if (QFileInfo(path).isFile()) {
        const DesktopEntry parsed = parseDesktopFile(path);
        if (!parsed.icon.isEmpty()) {
            return parsed.icon;
        }
    }
    return QStringLiteral("application-x-executable");
}

void DefaultAppsController::refresh()
{
    emit categoriesChanged();
}

void DefaultAppsController::openSystemSettings()
{
    const QStringList candidates = {
        QStringLiteral("systemsettings6"),
        QStringLiteral("systemsettings5"),
        QStringLiteral("systemsettings"),
        QStringLiteral("gnome-control-center"),
        QStringLiteral("xfce4-settings-manager"),
        QStringLiteral("mate-control-center"),
        QStringLiteral("lxqt-config"),
        QStringLiteral("stellar-settings"),
    };
    for (const QString &program : candidates) {
        const QString path = QStandardPaths::findExecutable(program);
        if (!path.isEmpty() && Stellar::startDetached(path, {}, QDir::homePath())) {
            return;
        }
    }
    emit errorOccurred(tr("No se encontró el panel de configuración del sistema"));
}
