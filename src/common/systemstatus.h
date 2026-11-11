#pragma once

#include <QObject>
#include <QTimer>

/*!
 * Aggregates the little pieces of information shown in the system tray:
 * connectivity and battery state. Audio lives in SystemManager.
 */
class SystemStatus : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool online READ online NOTIFY statusChanged)
    Q_PROPERTY(QString networkLabel READ networkLabel NOTIFY statusChanged)
    Q_PROPERTY(bool hasBattery READ hasBattery NOTIFY statusChanged)
    Q_PROPERTY(int batteryLevel READ batteryLevel NOTIFY statusChanged)
    Q_PROPERTY(bool charging READ charging NOTIFY statusChanged)

public:
    explicit SystemStatus(QObject *parent = nullptr);

    bool online() const { return m_online; }
    QString networkLabel() const { return m_networkLabel; }
    bool hasBattery() const { return m_hasBattery; }
    int batteryLevel() const { return m_batteryLevel; }
    bool charging() const { return m_charging; }

    Q_INVOKABLE void refresh();

signals:
    void statusChanged();

private:
    void refreshNetwork();
    void refreshBattery();
    void updateNetworkLabel();

    QTimer m_timer;
    bool m_online = true;
    QString m_networkLabel = QStringLiteral("En línea");
    bool m_hasBattery = false;
    int m_batteryLevel = -1;
    bool m_charging = false;
};
