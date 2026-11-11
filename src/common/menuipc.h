#pragma once

#include <QObject>

class QLocalServer;
class QLocalSocket;

/*!
 * Returns the per-user Unix socket name used to toggle the start menu.
 */
QString menuSocketName();

/*!
 * Server side, runs inside StellarMenu.
 */
class MenuServer : public QObject
{
    Q_OBJECT

public:
    explicit MenuServer(QObject *parent = nullptr);

    bool listen();
    bool isListening() const;

signals:
    void toggleRequested();
    void showRequested();
    void hideRequested();
    void quitRequested();

private:
    void handleConnection();

    QLocalServer *m_server = nullptr;
};

/*!
 * Client side, runs inside StellarPanel. If the menu is not running it is
 * spawned from the same installation directory (or PATH) and toggled after
 * a short delay.
 */
class MenuClient : public QObject
{
    Q_OBJECT

public:
    explicit MenuClient(QObject *parent = nullptr);

    Q_INVOKABLE void toggle();
    Q_INVOKABLE void showMenu();
    Q_INVOKABLE void hideMenu();
    Q_INVOKABLE void quitMenu();

private:
    bool sendCommand(const QByteArray &command);
    bool startMenuProcess();
    void scheduleShowRetry(int generation, int attempt);

    bool m_spawned = false;
    int m_generation = 0;
};
