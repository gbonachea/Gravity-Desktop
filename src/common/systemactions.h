#pragma once

#include <QObject>
#include <QStringList>

/*!
 * Shell-wide convenience actions used by the right column of the start menu.
 */
class SystemActions : public QObject
{
    Q_OBJECT

public:
    explicit SystemActions(QObject *parent = nullptr);

    Q_INVOKABLE void openDocuments();
    Q_INVOKABLE void openComputer();
    Q_INVOKABLE void openControlPanel();
    Q_INVOKABLE void openSettings();
    Q_INVOKABLE void openTerminal();
    Q_INVOKABLE bool launchFirstAvailable(const QStringList &programs);

signals:
    void failed(const QString &message);

private:
    bool launchWithArgs(const QString &program, const QStringList &arguments = {});
};
