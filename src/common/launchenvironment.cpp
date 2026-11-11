#include "launchenvironment.h"

#include <QProcess>

namespace Stellar
{

QProcessEnvironment launchEnvironment()
{
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.remove(QStringLiteral("QT_WAYLAND_SHELL_INTEGRATION"));
    return environment;
}

bool startDetached(const QString &program, const QStringList &arguments,
                   const QString &workingDirectory)
{
    if (program.trimmed().isEmpty()) {
        return false;
    }

    QProcess process;
    process.setProgram(program);
    process.setArguments(arguments);
    if (!workingDirectory.isEmpty()) {
        process.setWorkingDirectory(workingDirectory);
    }
    process.setProcessEnvironment(launchEnvironment());
    return process.startDetached();
}

} // namespace Stellar
