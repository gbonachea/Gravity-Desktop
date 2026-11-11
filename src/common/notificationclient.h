#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class QTimer;
class QDBusPendingCallWatcher;

/*!
 * Cliente del demonio stellar-notifications. Mantiene una copia local del
 * historial y expone la notificación actual para las widgets y el panel.
 */
class NotificationClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY changed)
    Q_PROPERTY(QVariantList items READ items NOTIFY changed)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex
                   NOTIFY currentChanged)
    Q_PROPERTY(QVariantMap current READ current NOTIFY currentChanged)
    Q_PROPERTY(bool available READ available NOTIFY changed)

public:
    explicit NotificationClient(QObject *parent = nullptr);

    int count() const { return int(m_items.size()); }
    QVariantList items() const { return m_items; }
    int currentIndex() const { return m_currentIndex; }
    QVariantMap current() const;
    bool available() const { return m_available; }

    void setCurrentIndex(int index);
    Q_INVOKABLE void select(int index) { setCurrentIndex(index); }
    Q_SLOT void refresh();
    Q_INVOKABLE void clear();
    Q_INVOKABLE void dismiss(uint id);

signals:
    void changed();
    void currentChanged();
    void notificationAdded(const QVariantMap &notification);

private Q_SLOTS:
    void onAdded(const QVariantMap &notification);
    void onRemoved(uint id);

private:
    void setItems(const QVariantList &items);
    void callCustom(const QString &method, const QVariantList &arguments = {});

    QVariantList m_items;
    int m_currentIndex = -1;
    bool m_available = false;
    bool m_refreshing = false;
    QTimer *m_retry = nullptr;
};
