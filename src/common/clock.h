#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

/*!
 * Locale-aware clock used by both the taskbar and the clock widget.
 */
class Clock : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString time READ timeText NOTIFY timeChanged)
    Q_PROPERTY(QString period READ periodText NOTIFY timeChanged)
    Q_PROPERTY(QString dateText READ dateText NOTIFY timeChanged)
    Q_PROPERTY(QString weekday READ weekdayText NOTIFY timeChanged)
    Q_PROPERTY(bool use24Hour READ use24Hour WRITE setUse24Hour NOTIFY use24HourChanged)

public:
    explicit Clock(QObject *parent = nullptr);

    QString timeText() const;
    QString periodText() const;
    QString dateText() const;
    QString weekdayText() const;

    bool use24Hour() const { return m_use24Hour; }
    void setUse24Hour(bool value);
    Q_INVOKABLE void toggleFormat();

signals:
    void timeChanged();
    void use24HourChanged();

private:
    void update();
    void loadFormat();
    void saveFormat() const;

    QTimer m_timer;
    bool m_use24Hour = false;
    QString m_timeText;
    QString m_periodText;
    QString m_dateText;
    QString m_weekdayText;
};
