#include "systemstatus.h"

#include <QDir>
#include <QFile>
#include <QNetworkInformation>

namespace
{

QString readFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromLatin1(file.readAll()).trimmed();
}

} // namespace

SystemStatus::SystemStatus(QObject *parent)
    : QObject(parent)
{
    if (QNetworkInformation::loadDefaultBackend()) {
        auto *information = QNetworkInformation::instance();
        if (information) {
            connect(information, &QNetworkInformation::reachabilityChanged, this,
                    [this] { refreshNetwork(); });
        }
    }

    m_timer.setInterval(5000);
    connect(&m_timer, &QTimer::timeout, this, &SystemStatus::refresh);
    m_timer.start();

    refresh();
}

void SystemStatus::refresh()
{
    refreshNetwork();
    refreshBattery();
}

void SystemStatus::refreshNetwork()
{
    bool online = true;
    if (QNetworkInformation::instance()) {
        online = QNetworkInformation::instance()->reachability()
            == QNetworkInformation::Reachability::Online;
    }
    if (online != m_online) {
        m_online = online;
        updateNetworkLabel();
        emit statusChanged();
    }
}

void SystemStatus::updateNetworkLabel()
{
    m_networkLabel = m_online ? tr("Wi-Fi conectado") : tr("Sin conexión");
}

void SystemStatus::refreshBattery()
{
    const QDir powerSupplies(QStringLiteral("/sys/class/power_supply"));
    const QStringList entries = powerSupplies.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    bool found = false;
    int level = -1;
    bool charging = false;

    for (const QString &entry : entries) {
        const QString base = powerSupplies.filePath(entry);
        if (readFile(base + QStringLiteral("/type")).compare(QLatin1String("Battery"),
                                                             Qt::CaseInsensitive)
            != 0) {
            continue;
        }

        found = true;
        const QString capacity = readFile(base + QStringLiteral("/capacity"));
        if (!capacity.isEmpty()) {
            level = qBound(0, capacity.toInt(), 100);
        }

        const QString status = readFile(base + QStringLiteral("/status"));
        charging = status.compare(QLatin1String("Charging"), Qt::CaseInsensitive) == 0
            || status.compare(QLatin1String("Full"), Qt::CaseInsensitive) == 0;
        break;
    }

    if (found != m_hasBattery || level != m_batteryLevel || charging != m_charging) {
        m_hasBattery = found;
        m_batteryLevel = level;
        m_charging = charging;
        emit statusChanged();
    }
}
