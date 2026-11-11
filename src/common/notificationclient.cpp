#include "notificationclient.h"

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusServiceWatcher>
#include <QTimer>

namespace
{
constexpr auto StellarService = "org.stellar.Notifications";
constexpr auto StellarPath = "/org/stellar/Notifications";
constexpr auto StellarInterface = "org.stellar.Notifications";
} // namespace

NotificationClient::NotificationClient(QObject *parent)
    : QObject(parent)
{
    QDBusConnection bus = QDBusConnection::sessionBus();

    bus.connect(QString::fromLatin1(StellarService), QString::fromLatin1(StellarPath),
                QString::fromLatin1(StellarInterface), QStringLiteral("Changed"),
                this, SLOT(refresh()));
    bus.connect(QString::fromLatin1(StellarService), QString::fromLatin1(StellarPath),
                QString::fromLatin1(StellarInterface), QStringLiteral("Added"),
                this, SLOT(onAdded(QVariantMap)));
    bus.connect(QString::fromLatin1(StellarService), QString::fromLatin1(StellarPath),
                QString::fromLatin1(StellarInterface), QStringLiteral("Removed"),
                this, SLOT(onRemoved(uint)));

    auto *watcher = new QDBusServiceWatcher(QString::fromLatin1(StellarService), bus,
                                            QDBusServiceWatcher::WatchForOwnerChange,
                                            this);
    connect(watcher, &QDBusServiceWatcher::serviceOwnerChanged, this,
            [this](const QString &, const QString &, const QString &) {
                m_available = false;
                refresh();
            });

    m_retry = new QTimer(this);
    m_retry->setInterval(3000);
    connect(m_retry, &QTimer::timeout, this, [this] {
        if (!m_available) {
            refresh();
        }
    });
    m_retry->start();

    QTimer::singleShot(0, this, &NotificationClient::refresh);
}

QVariantMap NotificationClient::current() const
{
    if (m_currentIndex < 0 || m_currentIndex >= m_items.size()) {
        return {};
    }
    return m_items.at(m_currentIndex).toMap();
}

void NotificationClient::setCurrentIndex(int index)
{
    if (index < -1 || index >= m_items.size()) {
        return;
    }
    if (index == m_currentIndex) {
        return;
    }
    m_currentIndex = index;
    emit currentChanged();
}

void NotificationClient::refresh()
{
    if (m_refreshing) {
        return;
    }
    m_refreshing = true;

    QDBusMessage message = QDBusMessage::createMethodCall(
        QString::fromLatin1(StellarService), QString::fromLatin1(StellarPath),
        QString::fromLatin1(StellarInterface), QStringLiteral("GetNotifications"));

    auto *watcher = new QDBusPendingCallWatcher(
        QDBusConnection::sessionBus().asyncCall(message), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this, watcher](QDBusPendingCallWatcher *call) {
                call->deleteLater();
                m_refreshing = false;

                const QDBusPendingReply<QVariantList> reply = *call;
                if (reply.isError()) {
                    m_available = false;
                    return;
                }
                m_available = true;

                QVariantList items;
                const QVariantList raw = reply.value();
                items.reserve(raw.size());
                for (const QVariant &value : raw) {
                    if (value.canConvert<QDBusArgument>()) {
                        QVariantMap map;
                        const QDBusArgument argument = value.value<QDBusArgument>();
                        argument >> map;
                        items.append(map);
                    } else {
                        items.append(value.toMap());
                    }
                }
                setItems(items);
            });
}

void NotificationClient::setItems(const QVariantList &items)
{
    if (items == m_items) {
        return;
    }

    m_items = items;
    if (m_items.isEmpty()) {
        m_currentIndex = -1;
    } else {
        // La notificación más reciente es la que se muestra; las anteriores
        // quedan disponibles en los puntos del widget.
        m_currentIndex = m_items.size() - 1;
    }

    emit changed();
    emit currentChanged();
}

void NotificationClient::clear()
{
    // Respuesta inmediata en la UI; el demonio confirma con Changed.
    m_items.clear();
    m_currentIndex = -1;
    emit changed();
    emit currentChanged();
    callCustom(QStringLiteral("Clear"));
}

void NotificationClient::dismiss(uint id)
{
    callCustom(QStringLiteral("Dismiss"), {QVariant::fromValue(id)});
}

void NotificationClient::callCustom(const QString &method, const QVariantList &arguments)
{
    QDBusMessage message = QDBusMessage::createMethodCall(
        QString::fromLatin1(StellarService), QString::fromLatin1(StellarPath),
        QString::fromLatin1(StellarInterface), method);
    if (!arguments.isEmpty()) {
        message.setArguments(arguments);
    }
    QDBusConnection::sessionBus().asyncCall(message);
}

void NotificationClient::onAdded(const QVariantMap &notification)
{
    emit notificationAdded(notification);
    refresh();
}

void NotificationClient::onRemoved(uint id)
{
    Q_UNUSED(id);
    refresh();
}
