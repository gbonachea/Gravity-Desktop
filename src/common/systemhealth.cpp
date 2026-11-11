#include "systemhealth.h"

#include <QDir>
#include <QFile>
#include <QNetworkInformation>
#include <QStorageInfo>

namespace
{

QString readSysFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromLatin1(file.readAll()).trimmed();
}

} // namespace

SystemHealth::SystemHealth(QObject *parent)
    : QObject(parent)
{
    m_timer.setInterval(30000);
    connect(&m_timer, &QTimer::timeout, this, &SystemHealth::update);
    m_timer.start();

    update();
}

void SystemHealth::refresh()
{
    update();
}

void SystemHealth::update()
{
    bool healthy = true;
    QString title = tr("Todo funciona correctamente");
    QString detail = tr("Almacenamiento, red y batería en orden");

    // Storage: warn under 10% free.
    QStorageInfo storage = QStorageInfo::root();
    storage.refresh();
    if (storage.isValid() && storage.bytesTotal() > 0) {
        const double freeRatio = double(storage.bytesAvailable()) / double(storage.bytesTotal());
        if (freeRatio < 0.10) {
            healthy = false;
            title = tr("Poco espacio en disco");
            detail = tr("Libera espacio para mantener el sistema estable");
        }
    }

    // Battery: warn under 10% when discharging.
    const QDir powerSupplies(QStringLiteral("/sys/class/power_supply"));
    const QStringList entries = powerSupplies.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &entry : entries) {
        const QString base = powerSupplies.filePath(entry);
        if (readSysFile(base + QStringLiteral("/type")) != QLatin1String("Battery")) {
            continue;
        }
        const int capacity = readSysFile(base + QStringLiteral("/capacity")).toInt();
        const QString status = readSysFile(base + QStringLiteral("/status"));
        if (capacity > 0 && capacity < 10 && status.compare(QLatin1String("Discharging"),
                                                            Qt::CaseInsensitive)
                == 0) {
            healthy = false;
            title = tr("Batería baja");
            detail = tr("Conecta el cargador para evitar suspender el equipo");
        }
        break;
    }

    if (QNetworkInformation::instance()
        && QNetworkInformation::instance()->reachability()
            == QNetworkInformation::Reachability::Disconnected) {
        healthy = false;
        title = tr("Sin conexión de red");
        detail = tr("Revisa el Wi-Fi o el cable de red");
    }

    if (healthy != m_healthy || title != m_title || detail != m_detail) {
        m_healthy = healthy;
        m_title = title;
        m_detail = detail;
        emit healthChanged();
    }
}
