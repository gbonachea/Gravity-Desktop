#include "notificationserver.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDateTime>
#include <QDebug>

namespace
{
constexpr auto FreedesktopService = "org.freedesktop.Notifications";
constexpr auto FreedesktopPath = "/org/freedesktop/Notifications";
constexpr auto StellarService = "org.stellar.Notifications";
constexpr auto StellarPath = "/org/stellar/Notifications";
constexpr int MaxHistory = 64;
} // namespace

// ---------------------------------------------------------------------------
// Adaptadores
// ---------------------------------------------------------------------------

FreedesktopNotificationsAdaptor::FreedesktopNotificationsAdaptor(NotificationServer *server,
                                                                 QObject *parent)
    : QDBusAbstractAdaptor(parent)
    , m_server(server)
{
}

QStringList FreedesktopNotificationsAdaptor::GetCapabilities() const
{
    return {QStringLiteral("body"), QStringLiteral("body-markup"),
            QStringLiteral("icon-static"), QStringLiteral("persistence")};
}

uint FreedesktopNotificationsAdaptor::Notify(
    const QString &appName, uint replacesId, const QString &appIcon,
    const QString &summary, const QString &body, const QStringList &actions,
    const QVariantMap &hints, int expireTimeout)
{
    return m_server->notify(appName, replacesId, appIcon, summary, body, actions,
                            hints, expireTimeout);
}

void FreedesktopNotificationsAdaptor::CloseNotification(uint id)
{
    m_server->closeNotification(id, 3);
}

QString FreedesktopNotificationsAdaptor::GetServerInformation(
    QString &vendor, QString &version, QString &specVersion) const
{
    vendor = QStringLiteral("Stellar Shell");
    version = QStringLiteral("0.1.0");
    specVersion = QStringLiteral("1.2");
    return QStringLiteral("Stellar Notifications");
}

StellarNotificationsAdaptor::StellarNotificationsAdaptor(NotificationServer *server,
                                                         QObject *parent)
    : QDBusAbstractAdaptor(parent)
    , m_server(server)
{
}

QVariantList StellarNotificationsAdaptor::GetNotifications() const
{
    return m_server->notifications();
}

void StellarNotificationsAdaptor::Clear()
{
    m_server->clearNotifications();
}

void StellarNotificationsAdaptor::Dismiss(uint id)
{
    m_server->closeNotification(id, 2);
}

// ---------------------------------------------------------------------------
// Servidor
// ---------------------------------------------------------------------------

NotificationServer::NotificationServer(QObject *parent)
    : QObject(parent)
{
}

bool NotificationServer::start()
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        qWarning() << "stellar-notifications: no hay bus de sesión D-Bus";
        return false;
    }

    m_freedesktop = new FreedesktopNotificationsAdaptor(this, this);
    m_stellar = new StellarNotificationsAdaptor(this, this);

    const auto flags = QDBusConnection::ExportAllSlots
        | QDBusConnection::ExportAllSignals;
    if (!bus.registerObject(QString::fromLatin1(FreedesktopPath), m_freedesktop, flags)) {
        qWarning() << "stellar-notifications: no se pudo exportar"
                   << FreedesktopPath;
        return false;
    }
    if (!bus.registerObject(QString::fromLatin1(StellarPath), m_stellar, flags)) {
        qWarning() << "stellar-notifications: no se pudo exportar" << StellarPath;
        return false;
    }

    QDBusConnectionInterface *iface = bus.interface();
    if (!iface) {
        return false;
    }

    // La interfaz propia es la que consumen el panel y los widgets.
    const QDBusReply<QDBusConnectionInterface::RegisterServiceReply> custom =
        iface->registerService(QString::fromLatin1(StellarService),
                               QDBusConnectionInterface::DontQueueService);
    if (!custom.isValid()
        || custom.value() != QDBusConnectionInterface::ServiceRegistered) {
        qWarning() << "stellar-notifications: ya hay otro demonio Stellar activo";
        return false;
    }

    // Si otro demonio (dunst, mako, plasma...) ya posee el nombre estándar,
    // no se le quita: el shell seguirá publicando su historial propio.
    const QDBusReply<QDBusConnectionInterface::RegisterServiceReply> standard =
        iface->registerService(QString::fromLatin1(FreedesktopService),
                               QDBusConnectionInterface::DontQueueService);
    if (!standard.isValid()
        || standard.value() != QDBusConnectionInterface::ServiceRegistered) {
        qWarning() << "stellar-notifications: otro servicio de notificaciones está"
                      " activo; se usa solo la interfaz Stellar";
    }

    return true;
}

QStringList NotificationServer::capabilities() const
{
    return {QStringLiteral("body"), QStringLiteral("body-markup"),
            QStringLiteral("icon-static"), QStringLiteral("persistence")};
}

uint NotificationServer::notify(const QString &appName, uint replacesId,
                                const QString &appIcon, const QString &summary,
                                const QString &body, const QStringList &actions,
                                const QVariantMap &hints, int expireTimeout)
{
    Q_UNUSED(expireTimeout);

    Entry entry;
    entry.appName = appName;
    entry.appIcon = appIcon;
    if (entry.appIcon.isEmpty()) {
        entry.appIcon = hints.value(QStringLiteral("desktop-entry")).toString();
    }
    entry.summary = summary;
    entry.body = body;
    entry.urgency = hints.value(QStringLiteral("urgency")).toInt();
    if (entry.urgency <= 0 || entry.urgency > 2) {
        entry.urgency = 1;
    }
    entry.timestamp = QDateTime::currentMSecsSinceEpoch();
    entry.actions = actions;

    int existing = replacesId != 0 ? indexOf(replacesId) : -1;
    if (existing >= 0) {
        entry.id = replacesId;
        m_entries[existing] = entry;
    } else {
        entry.id = m_nextId++;
        m_entries.append(entry);
        if (m_entries.size() > MaxHistory) {
            const uint removed = m_entries.first().id;
            m_entries.removeFirst();
            notifyRemoved(removed);
        }
    }

    notifyAdded(entry);
    notifyChanged();
    return entry.id;
}

void NotificationServer::closeNotification(uint id, uint reason)
{
    const int index = indexOf(id);
    if (index < 0) {
        return;
    }
    m_entries.removeAt(index);
    if (m_freedesktop) {
        Q_EMIT m_freedesktop->NotificationClosed(id, reason);
    }
    QDBusMessage signal = QDBusMessage::createSignal(
        QString::fromLatin1(FreedesktopPath),
        QStringLiteral("org.freedesktop.Notifications"),
        QStringLiteral("NotificationClosed"));
    signal << id << reason;
    QDBusConnection::sessionBus().send(signal);
    notifyRemoved(id);
    notifyChanged();
}

void NotificationServer::clearNotifications()
{
    if (m_entries.isEmpty()) {
        return;
    }

    const QVector<Entry> previous = m_entries;
    m_entries.clear();
    for (const Entry &entry : previous) {
        if (m_freedesktop) {
            Q_EMIT m_freedesktop->NotificationClosed(entry.id, 2);
        }
        QDBusMessage closed = QDBusMessage::createSignal(
            QString::fromLatin1(FreedesktopPath),
            QStringLiteral("org.freedesktop.Notifications"),
            QStringLiteral("NotificationClosed"));
        closed << entry.id << uint(2);
        QDBusConnection::sessionBus().send(closed);

        if (m_stellar) {
            Q_EMIT m_stellar->Removed(entry.id);
        }
    }
    notifyChanged();
}

QVariantList NotificationServer::notifications() const
{
    QVariantList result;
    result.reserve(m_entries.size());
    for (const Entry &entry : m_entries) {
        result.append(toMap(entry));
    }
    return result;
}

QVariantMap NotificationServer::toMap(const Entry &entry) const
{
    return {
        {QStringLiteral("id"), entry.id},
        {QStringLiteral("appName"), entry.appName},
        {QStringLiteral("appIcon"), entry.appIcon},
        {QStringLiteral("summary"), entry.summary},
        {QStringLiteral("body"), entry.body},
        {QStringLiteral("urgency"), entry.urgency},
        {QStringLiteral("timestamp"), entry.timestamp},
        {QStringLiteral("actions"), entry.actions},
    };
}

int NotificationServer::indexOf(uint id) const
{
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries.at(i).id == id) {
            return i;
        }
    }
    return -1;
}

void NotificationServer::notifyChanged()
{
    if (m_stellar) {
        Q_EMIT m_stellar->Changed();
    }
    QDBusMessage signal = QDBusMessage::createSignal(
        QString::fromLatin1(StellarPath), QStringLiteral("org.stellar.Notifications"),
        QStringLiteral("Changed"));
    QDBusConnection::sessionBus().send(signal);
}

void NotificationServer::notifyAdded(const Entry &entry)
{
    const QVariantMap map = toMap(entry);
    if (m_stellar) {
        Q_EMIT m_stellar->Added(map);
    }
    QDBusMessage signal = QDBusMessage::createSignal(
        QString::fromLatin1(StellarPath), QStringLiteral("org.stellar.Notifications"),
        QStringLiteral("Added"));
    signal << map;
    QDBusConnection::sessionBus().send(signal);
}

void NotificationServer::notifyRemoved(uint id)
{
    if (m_stellar) {
        Q_EMIT m_stellar->Removed(id);
    }
    QDBusMessage signal = QDBusMessage::createSignal(
        QString::fromLatin1(StellarPath), QStringLiteral("org.stellar.Notifications"),
        QStringLiteral("Removed"));
    signal << id;
    QDBusConnection::sessionBus().send(signal);
}
