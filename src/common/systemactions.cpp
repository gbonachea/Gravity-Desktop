#include "systemactions.h"

#include "filemanager.h"
#include "launchenvironment.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

SystemActions::SystemActions(QObject *parent)
    : QObject(parent)
{
}

void SystemActions::openDocuments()
{
    const QString documents =
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString target = QFileInfo::exists(documents) ? documents : QDir::homePath();
    if (!Stellar::openWithFileManager(target)) {
        emit failed(tr("No se pudo abrir la carpeta de documentos"));
    }
}

void SystemActions::openComputer()
{
    if (!Stellar::openWithFileManager(QStringLiteral("/"))) {
        emit failed(tr("No se pudo abrir el explorador de archivos"));
    }
}

void SystemActions::openControlPanel()
{
    launchFirstAvailable({
        QStringLiteral("stellar-settings"),
        QStringLiteral("systemsettings"),
        QStringLiteral("systemsettings6"),
        QStringLiteral("gnome-control-center"),
        QStringLiteral("xfce4-settings-manager"),
        QStringLiteral("mate-control-center"),
        QStringLiteral("lxqt-config"),
    });
}

void SystemActions::openSettings()
{
    // "Configuración" abre los ajustes del sistema; el Centro de Stellar
    // queda reservado al acceso "Panel de control". Se prueban primero los
    // paneles nativos (KDE/Plasma, GNOME, Xfce...) y KWin como último recurso
    // antes de caer en el Centro de Stellar.
    const QStringList candidates = {
        QStringLiteral("systemsettings6"),
        QStringLiteral("systemsettings5"),
        QStringLiteral("systemsettings"),
        QStringLiteral("gnome-control-center"),
        QStringLiteral("xfce4-settings-manager"),
        QStringLiteral("mate-control-center"),
        QStringLiteral("lxqt-config"),
        QStringLiteral("cinnamon-settings"),
        QStringLiteral("stellar-settings"),
    };

    const QString appDir = QCoreApplication::applicationDirPath();
    for (const QString &program : candidates) {
        const QString local = appDir + QLatin1Char('/') + program;
        if (QFileInfo(local).isExecutable()) {
            if (launchWithArgs(local)) {
                return;
            }
        }
        const QString path = QStandardPaths::findExecutable(program);
        if (!path.isEmpty() && launchWithArgs(path)) {
            return;
        }
    }

    const QString kcmshell = QStandardPaths::findExecutable(QStringLiteral("kcmshell6"));
    if (!kcmshell.isEmpty() && launchWithArgs(kcmshell, {QStringLiteral("kwincompositing")})) {
        return;
    }

    emit failed(tr("No se encontró ninguna aplicación de configuración"));
}

void SystemActions::openTerminal()
{
    launchFirstAvailable({
        QStringLiteral("x-terminal-emulator"),
        QStringLiteral("kgx"),
        QStringLiteral("gnome-terminal"),
        QStringLiteral("konsole"),
        QStringLiteral("alacritty"),
        QStringLiteral("foot"),
        QStringLiteral("xterm"),
    });
}

bool SystemActions::launchFirstAvailable(const QStringList &programs)
{
    const QString appDir = QCoreApplication::applicationDirPath();
    for (const QString &program : programs) {
        // Preferir el binario hermano (árbol de compilación o instalación).
        const QString local = appDir + QLatin1Char('/') + program;
        if (QFileInfo::exists(local) && QFileInfo(local).isExecutable()) {
            return launchWithArgs(local);
        }
        const QString path = QStandardPaths::findExecutable(program);
        if (!path.isEmpty()) {
            return launchWithArgs(path);
        }
    }

    emit failed(tr("No se encontró ninguna aplicación compatible"));
    return false;
}

bool SystemActions::launchWithArgs(const QString &program, const QStringList &arguments)
{
    if (!Stellar::startDetached(program, arguments, QDir::homePath())) {
        emit failed(tr("No se pudo iniciar %1").arg(program));
        return false;
    }
    return true;
}
