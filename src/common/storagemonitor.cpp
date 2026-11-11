#include "storagemonitor.h"

#include <QLocale>

StorageMonitor::StorageMonitor(QObject *parent)
    : QObject(parent)
{
    m_timer.setInterval(30000);
    connect(&m_timer, &QTimer::timeout, this, &StorageMonitor::update);
    m_timer.start();

    update();
}

void StorageMonitor::refresh()
{
    update();
}

void StorageMonitor::update()
{
    QStorageInfo storage = QStorageInfo::root();
    storage.refresh();

    m_available = storage.isValid() && storage.isReady() && storage.bytesTotal() > 0;
    if (!m_available) {
        m_name = tr("Disco local");
        m_mountPath = QStringLiteral("/");
        m_freeText = QStringLiteral("--");
        m_usedText = QStringLiteral("--");
        m_totalText = QStringLiteral("--");
        m_percentUsed = 0.0;
        emit statsChanged();
        return;
    }

    const qint64 total = storage.bytesTotal();
    const qint64 free = storage.bytesAvailable();
    const qint64 used = qMax<qint64>(0, total - free);

    const QString displayName = storage.displayName();
    m_name = displayName.isEmpty() ? tr("Disco local") : displayName;
    m_mountPath = storage.rootPath();

    const QLocale locale = QLocale::system();
    m_freeText = locale.formattedDataSize(free, 1, QLocale::DataSizeTraditionalFormat);
    m_usedText = locale.formattedDataSize(used, 1, QLocale::DataSizeTraditionalFormat);
    m_totalText = locale.formattedDataSize(total, 1, QLocale::DataSizeTraditionalFormat);
    m_percentUsed = total > 0 ? (qreal(used) / qreal(total)) * 100.0 : 0.0;

    emit statsChanged();
}
