#include "applauncher.h"

#include "desktopentrymodel.h"
#include "launchenvironment.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace
{

bool isFieldCodeToken(const QString &token)
{
    return token == QLatin1String("%f") || token == QLatin1String("%F")
        || token == QLatin1String("%u") || token == QLatin1String("%U")
        || token == QLatin1String("%i");
}

QString replaceFieldCodes(QString token, const QString &name, const QString &desktopFile)
{
    token.replace(QLatin1String("%%"), QStringLiteral("\x01"));
    token.replace(QLatin1String("%c"), name);
    token.replace(QLatin1String("%k"), desktopFile);
    token.remove(QLatin1String("%f"));
    token.remove(QLatin1String("%F"));
    token.remove(QLatin1String("%u"));
    token.remove(QLatin1String("%U"));
    token.remove(QLatin1String("%i"));
    token.replace(QStringLiteral("\x01"), QStringLiteral("%"));
    return token;
}

} // namespace

AppLauncher::AppLauncher(QObject *parent)
    : QObject(parent)
{
}

bool AppLauncher::launchDesktopFile(const QString &desktopFile)
{
    const DesktopEntry *entry = DesktopEntryIndex::instance().findByDesktopFile(desktopFile);
    if (!entry) {
        // Los accesos anclados al escritorio viven en ~/Escritorio y no están
        // en el índice XDG. Si la ruta existe, la procesamos directamente.
        if (QFileInfo(desktopFile).isFile()) {
            const DesktopEntry parsed = parseDesktopFile(desktopFile);
            if (isValidApplication(parsed)) {
                return launchEntry(parsed);
            }
        }
        emit launchFailed(tr("No se encontró la entrada de escritorio: %1").arg(desktopFile));
        return false;
    }
    return launchEntry(*entry);
}

bool AppLauncher::launchCommand(const QString &commandLine)
{
    if (commandLine.trimmed().isEmpty()) {
        return false;
    }

    const bool ok = Stellar::startDetached(QStringLiteral("/bin/sh"),
                                           {QStringLiteral("-c"), commandLine},
                                           QDir::homePath());
    if (!ok) {
        emit launchFailed(tr("No se pudo ejecutar el comando"));
    }
    return ok;
}

bool AppLauncher::launchProgram(const QString &program, const QStringList &arguments)
{
    if (program.trimmed().isEmpty()) {
        return false;
    }

    const bool ok = Stellar::startDetached(program, arguments, QDir::homePath());
    if (!ok) {
        emit launchFailed(tr("No se pudo iniciar %1").arg(program));
    }
    return ok;
}

bool AppLauncher::launchEntry(const DesktopEntry &entry)
{
    QStringList arguments = expandExec(entry.exec, entry.name,
                                       QFileInfo(entry.sourceFile).fileName());
    if (arguments.isEmpty()) {
        emit launchFailed(tr("La entrada %1 no contiene un comando válido").arg(entry.name));
        return false;
    }

    QString program = arguments.takeFirst();
    QString workingDirectory = entry.path;
    if (workingDirectory.isEmpty() || !QFileInfo(workingDirectory).isDir()) {
        workingDirectory = QDir::homePath();
    }

    if (entry.terminal) {
        const QString terminal = terminalProgram();
        if (terminal.isEmpty()) {
            emit launchFailed(tr("No se encontró un emulador de terminal"));
            return false;
        }

        const QString terminalName = QFileInfo(terminal).fileName();
        QStringList terminalArguments;
        if (terminalName.contains(QLatin1String("gnome-terminal"))
            || terminalName.contains(QLatin1String("kgx"))) {
            terminalArguments << QStringLiteral("--");
        } else if (terminalName == QLatin1String("xfce4-terminal")
                   || terminalName == QLatin1String("terminator")) {
            terminalArguments << QStringLiteral("-x");
        } else {
            terminalArguments << QStringLiteral("-e");
        }

        terminalArguments << program << arguments;
        program = terminal;
        arguments = terminalArguments;
    }

    const bool ok = Stellar::startDetached(program, arguments, workingDirectory);
    if (!ok) {
        emit launchFailed(tr("No se pudo iniciar %1").arg(entry.name));
        return false;
    }

    emit launched(entry.name);
    return true;
}

QStringList AppLauncher::tokenize(const QString &commandLine)
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

QStringList AppLauncher::expandExec(const QString &execLine, const QString &name,
                                    const QString &desktopFile)
{
    QStringList result;
    const QStringList tokens = tokenize(execLine);

    for (const QString &token : tokens) {
        if (isFieldCodeToken(token)) {
            continue;
        }

        QString expanded = replaceFieldCodes(token, name, desktopFile);
        if (expanded.isEmpty()) {
            continue;
        }
        result.append(expanded);
    }

    return result;
}

QString AppLauncher::terminalProgram()
{
    static const QStringList candidates = {
        QStringLiteral("x-terminal-emulator"),
        QStringLiteral("kgx"),
        QStringLiteral("gnome-terminal"),
        QStringLiteral("konsole"),
        QStringLiteral("alacritty"),
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
