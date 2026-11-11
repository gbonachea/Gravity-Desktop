#include "menuipc.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QLocalServer>
#include <QLocalSocket>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>

#include <unistd.h>

QString menuSocketName()
{
    return QStringLiteral("stellar-menu-%1").arg(getuid());
}

MenuServer::MenuServer(QObject *parent)
    : QObject(parent)
    , m_server(new QLocalServer(this))
{
    connect(m_server, &QLocalServer::newConnection, this, &MenuServer::handleConnection);
}

bool MenuServer::listen()
{
    const QString name = menuSocketName();

    if (m_server->listen(name)) {
        return true;
    }

    // Distinguish "another instance is alive" from a stale socket file.
    QLocalSocket probe;
    probe.connectToServer(name);
    if (probe.waitForConnected(150)) {
        probe.disconnectFromServer();
        return false;
    }

    QLocalServer::removeServer(name);
    return m_server->listen(name);
}

bool MenuServer::isListening() const
{
    return m_server->isListening();
}

void MenuServer::handleConnection()
{
    while (QLocalSocket *socket = m_server->nextPendingConnection()) {
        connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);

        if (!socket->canReadLine() && !socket->waitForReadyRead(100)) {
            socket->disconnectFromServer();
            continue;
        }

        const QByteArray payload = socket->readAll().trimmed();
        const QList<QByteArray> commands = payload.split('\n');
        for (const QByteArray &command : commands) {
            const QByteArray trimmed = command.trimmed();
            if (trimmed == "toggle") {
                emit toggleRequested();
            } else if (trimmed == "show") {
                emit showRequested();
            } else if (trimmed == "hide") {
                emit hideRequested();
            } else if (trimmed == "quit") {
                emit quitRequested();
            }
        }

        socket->disconnectFromServer();
    }
}

MenuClient::MenuClient(QObject *parent)
    : QObject(parent)
{
}

void MenuClient::toggle()
{
    ++m_generation;

    if (sendCommand(QByteArrayLiteral("toggle"))) {
        return;
    }

    startMenuProcess();

    // The menu may still be starting up. Retrying "show" is safe because it is
    // idempotent, unlike "toggle": a late retry can never close a menu that has
    // just opened. This removes the close/reopen flicker on cold starts.
    scheduleShowRetry(m_generation, 0);
}

void MenuClient::showMenu()
{
    ++m_generation;

    if (sendCommand(QByteArrayLiteral("show"))) {
        return;
    }

    startMenuProcess();
    scheduleShowRetry(m_generation, 0);
}

void MenuClient::hideMenu()
{
    ++m_generation;
    sendCommand(QByteArrayLiteral("hide"));
}

void MenuClient::quitMenu()
{
    ++m_generation;
    sendCommand(QByteArrayLiteral("quit"));
}

void MenuClient::scheduleShowRetry(int generation, int attempt)
{
    if (generation != m_generation || attempt >= 12) {
        return;
    }

    if (sendCommand(QByteArrayLiteral("show"))) {
        return;
    }

    QTimer::singleShot(250, this, [this, generation, attempt] {
        scheduleShowRetry(generation, attempt + 1);
    });
}

bool MenuClient::sendCommand(const QByteArray &command)
{
    QLocalSocket socket;
    socket.connectToServer(menuSocketName());
    if (!socket.waitForConnected(150)) {
        return false;
    }

    const QByteArray payload = command + '\n';
    if (socket.write(payload) != payload.size()) {
        socket.disconnectFromServer();
        return false;
    }

    // Un socket local entrega el comando en cuanto se hace flush. No usamos
    // waitForBytesWritten como condición de éxito: si devuelve false tras un
    // write correcto, toggle() interpretaría que el menú no está y enviaría un
    // "show" extra, provocando el cierre+reapertura instantáneo.
    socket.flush();
    socket.waitForBytesWritten(80);
    socket.disconnectFromServer();
    return true;
}

bool MenuClient::startMenuProcess()
{
    const QString local = QCoreApplication::applicationDirPath()
        + QStringLiteral("/stellar-menu");
    QString program;
    if (QFileInfo::exists(local)) {
        program = local;
    } else {
        program = QStandardPaths::findExecutable(QStringLiteral("stellar-menu"));
    }

    if (program.isEmpty()) {
        return false;
    }

    const bool wasSpawned = m_spawned;
    m_spawned = true;
    if (QProcess::startDetached(program, {})) {
        return !wasSpawned;
    }
    return false;
}
