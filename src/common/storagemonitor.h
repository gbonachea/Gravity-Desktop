#pragma once

#include <QObject>
#include <QStorageInfo>
#include <QString>
#include <QTimer>

/*!
 * Exposes the root filesystem statistics with human readable formatting.
 */
class StorageMonitor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name NOTIFY statsChanged)
    Q_PROPERTY(QString mountPath READ mountPath NOTIFY statsChanged)
    Q_PROPERTY(QString freeText READ freeText NOTIFY statsChanged)
    Q_PROPERTY(QString usedText READ usedText NOTIFY statsChanged)
    Q_PROPERTY(QString totalText READ totalText NOTIFY statsChanged)
    Q_PROPERTY(qreal percentUsed READ percentUsed NOTIFY statsChanged)
    Q_PROPERTY(bool available READ available NOTIFY statsChanged)

public:
    explicit StorageMonitor(QObject *parent = nullptr);

    QString name() const { return m_name; }
    QString mountPath() const { return m_mountPath; }
    QString freeText() const { return m_freeText; }
    QString usedText() const { return m_usedText; }
    QString totalText() const { return m_totalText; }
    qreal percentUsed() const { return m_percentUsed; }
    bool available() const { return m_available; }

    Q_INVOKABLE void refresh();

signals:
    void statsChanged();

private:
    void update();

    QTimer m_timer;
    QString m_name;
    QString m_mountPath;
    QString m_freeText;
    QString m_usedText;
    QString m_totalText;
    qreal m_percentUsed = 0.0;
    bool m_available = false;
};
