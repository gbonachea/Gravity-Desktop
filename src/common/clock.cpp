#include "clock.h"

#include <QDate>
#include <QLocale>
#include <QSettings>
#include <QTime>

namespace
{

QString capitalizeFirst(QString text)
{
    if (!text.isEmpty()) {
        text[0] = text.at(0).toUpper();
    }
    return text;
}

} // namespace

Clock::Clock(QObject *parent)
    : QObject(parent)
{
    loadFormat();

    m_timer.setInterval(1000);
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, &Clock::update);
    m_timer.start();

    update();
}

QString Clock::timeText() const
{
    return m_timeText;
}

QString Clock::periodText() const
{
    return m_periodText;
}

QString Clock::dateText() const
{
    return m_dateText;
}

QString Clock::weekdayText() const
{
    return m_weekdayText;
}

void Clock::setUse24Hour(bool value)
{
    if (m_use24Hour == value) {
        return;
    }
    m_use24Hour = value;
    saveFormat();
    emit use24HourChanged();
    update();
}

void Clock::toggleFormat()
{
    setUse24Hour(!m_use24Hour);
}

void Clock::loadFormat()
{
    QSettings settings;
    if (settings.contains(QStringLiteral("clock/use24Hour"))) {
        m_use24Hour = settings.value(QStringLiteral("clock/use24Hour"), false).toBool();
        return;
    }

    const QString format = QLocale::system().timeFormat(QLocale::ShortFormat);
    m_use24Hour = !format.contains(QLatin1Char('a'), Qt::CaseInsensitive);
}

void Clock::saveFormat() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("clock/use24Hour"), m_use24Hour);
}

void Clock::update()
{
    const QLocale locale = QLocale::system();
    const QTime now = QTime::currentTime();
    const QDate today = QDate::currentDate();

    const QString newTime = locale.toString(now, m_use24Hour ? QStringLiteral("HH:mm")
                                                             : QStringLiteral("h:mm"));
    const QString newPeriod = m_use24Hour
        ? QString()
        : (now.hour() < 12 ? locale.amText() : locale.pmText());

    const QString longDate = locale.toString(today, QLocale::LongFormat);
    const QString newDate = capitalizeFirst(longDate);
    const QString newWeekday = capitalizeFirst(locale.dayName(today.dayOfWeek(), QLocale::LongFormat));

    if (newTime == m_timeText && newPeriod == m_periodText && newDate == m_dateText
        && newWeekday == m_weekdayText) {
        return;
    }

    m_timeText = newTime;
    m_periodText = newPeriod;
    m_dateText = newDate;
    m_weekdayText = newWeekday;
    emit timeChanged();
}
