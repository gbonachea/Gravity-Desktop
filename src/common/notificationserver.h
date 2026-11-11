#pragma once

#include <QDBusAbstractAdaptor>
#include <QObject>
#include <QVariantList>
#include <QVector>

class NotificationServer;

/*!
 * Adaptador D-Bus estándar org.freedesktop.Notifications.
 */
class FreedesktopNotificationsAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Notifications")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.freedesktop.Notifications\">\n"
"    <method name=\"GetCapabilities\">\n"
"      <arg direction=\"out\" type=\"as\"/>\n"
"    </method>\n"
"    <method name=\"Notify\">\n"
"      <arg direction=\"in\" type=\"s\"/>\n"
"      <arg direction=\"in\" type=\"u\"/>\n"
"      <arg direction=\"in\" type=\"s\"/>\n"
"      <arg direction=\"in\" type=\"s\"/>\n"
"      <arg direction=\"in\" type=\"s\"/>\n"
"      <arg direction=\"in\" type=\"as\"/>\n"
"      <arg direction=\"in\" type=\"a{sv}\"/>\n"
"      <arg direction=\"in\" type=\"i\"/>\n"
"      <arg direction=\"out\" type=\"u\"/>\n"
"    </method>\n"
"    <method name=\"CloseNotification\">\n"
"      <arg direction=\"in\" type=\"u\"/>\n"
"    </method>\n"
"    <method name=\"GetServerInformation\">\n"
"      <arg direction=\"out\" type=\"s\"/>\n"
"      <arg direction=\"out\" type=\"s\"/>\n"
"      <arg direction=\"out\" type=\"s\"/>\n"
"      <arg direction=\"out\" type=\"s\"/>\n"
"    </method>\n"
"    <signal name=\"NotificationClosed\">\n"
"      <arg type=\"u\"/>\n"
"      <arg type=\"u\"/>\n"
"    </signal>\n"
"    <signal name=\"ActionInvoked\">\n"
"      <arg type=\"u\"/>\n"
"      <arg type=\"s\"/>\n"
"    </signal>\n"
"  </interface>\n")

public:
    explicit FreedesktopNotificationsAdaptor(NotificationServer *server, QObject *parent);

public Q_SLOTS:
    QStringList GetCapabilities() const;
    uint Notify(const QString &appName, uint replacesId, const QString &appIcon,
                const QString &summary, const QString &body,
                const QStringList &actions, const QVariantMap &hints,
                int expireTimeout);
    void CloseNotification(uint id);
    QString GetServerInformation(QString &vendor, QString &version,
                                 QString &specVersion) const;

Q_SIGNALS:
    void NotificationClosed(uint id, uint reason);
    void ActionInvoked(uint id, const QString &actionKey);

private:
    NotificationServer *m_server = nullptr;
};

/*!
 * Interfaz propia para que stellar-widgets y stellar-panel compartan el
 * historial de notificaciones del demonio.
 */
class StellarNotificationsAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.stellar.Notifications")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"org.stellar.Notifications\">\n"
"    <method name=\"GetNotifications\">\n"
"      <arg direction=\"out\" type=\"av\"/>\n"
"    </method>\n"
"    <method name=\"Clear\"/>\n"
"    <method name=\"Dismiss\">\n"
"      <arg direction=\"in\" type=\"u\"/>\n"
"    </method>\n"
"    <signal name=\"Changed\"/>\n"
"    <signal name=\"Added\">\n"
"      <arg type=\"a{sv}\"/>\n"
"    </signal>\n"
"    <signal name=\"Removed\">\n"
"      <arg type=\"u\"/>\n"
"    </signal>\n"
"  </interface>\n")

public:
    explicit StellarNotificationsAdaptor(NotificationServer *server, QObject *parent);

public Q_SLOTS:
    QVariantList GetNotifications() const;
    void Clear();
    void Dismiss(uint id);

Q_SIGNALS:
    void Changed();
    void Added(const QVariantMap &notification);
    void Removed(uint id);

private:
    NotificationServer *m_server = nullptr;
};

/*!
 * Demonio de notificaciones. Si no hay otro servidor en la sesión registra
 * org.freedesktop.Notifications; en cualquier caso publica la interfaz
 * org.stellar.Notifications usada por las superficies del shell.
 */
class NotificationServer : public QObject
{
    Q_OBJECT

public:
    explicit NotificationServer(QObject *parent = nullptr);

    bool start();
    int count() const { return int(m_entries.size()); }

    // Usados por los adaptadores.
    QStringList capabilities() const;
    uint notify(const QString &appName, uint replacesId, const QString &appIcon,
                const QString &summary, const QString &body,
                const QStringList &actions, const QVariantMap &hints,
                int expireTimeout);
    void closeNotification(uint id, uint reason);
    void clearNotifications();
    QVariantList notifications() const;

private:
    struct Entry {
        uint id = 0;
        QString appName;
        QString appIcon;
        QString summary;
        QString body;
        int urgency = 1;
        qint64 timestamp = 0;
        QStringList actions;
    };

    QVariantMap toMap(const Entry &entry) const;
    int indexOf(uint id) const;
    void notifyChanged();
    void notifyAdded(const Entry &entry);
    void notifyRemoved(uint id);

    QVector<Entry> m_entries;
    uint m_nextId = 1;
    FreedesktopNotificationsAdaptor *m_freedesktop = nullptr;
    StellarNotificationsAdaptor *m_stellar = nullptr;
};
