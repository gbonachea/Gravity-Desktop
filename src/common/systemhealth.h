#pragma once

#include <QObject>
#include <QTimer>

/*!
 * Summarizes the most important health checks for the desktop status card.
 */
class SystemHealth : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool healthy READ healthy NOTIFY healthChanged)
    Q_PROPERTY(QString title READ title NOTIFY healthChanged)
    Q_PROPERTY(QString detail READ detail NOTIFY healthChanged)

public:
    explicit SystemHealth(QObject *parent = nullptr);

    bool healthy() const { return m_healthy; }
    QString title() const { return m_title; }
    QString detail() const { return m_detail; }

    Q_INVOKABLE void refresh();

signals:
    void healthChanged();

private:
    void update();

    QTimer m_timer;
    bool m_healthy = true;
    QString m_title = QStringLiteral("Todo funciona correctamente");
    QString m_detail = QStringLiteral("Almacenamiento, red y batería en orden");
};
