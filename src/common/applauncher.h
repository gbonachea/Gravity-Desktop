#pragma once

#include <QObject>
#include <QStringList>

/*!
 * Launches desktop entries and shell commands without blocking the UI.
 */
class AppLauncher : public QObject
{
    Q_OBJECT

public:
    explicit AppLauncher(QObject *parent = nullptr);

    Q_INVOKABLE bool launchDesktopFile(const QString &desktopFile);
    Q_INVOKABLE bool launchCommand(const QString &commandLine);
    Q_INVOKABLE bool launchProgram(const QString &program, const QStringList &arguments = {});

signals:
    void launched(const QString &name);
    void launchFailed(const QString &message);

private:
    bool launchEntry(const struct DesktopEntry &entry);
    static QStringList tokenize(const QString &commandLine);
    static QStringList expandExec(const QString &execLine, const QString &name,
                                  const QString &desktopFile);
    static QString terminalProgram();
};
