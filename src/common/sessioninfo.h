#pragma once

#include <QObject>
#include <QString>

/*!
 * User identity used by the start menu header.
 */
class SessionInfo : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString userName READ userName CONSTANT)
    Q_PROPERTY(QString displayName READ displayName CONSTANT)
    Q_PROPERTY(QString initials READ initials CONSTANT)
    Q_PROPERTY(QString avatarSource READ avatarSource CONSTANT)
    Q_PROPERTY(QString sessionLabel READ sessionLabel CONSTANT)

public:
    explicit SessionInfo(QObject *parent = nullptr);

    QString userName() const { return m_userName; }
    QString displayName() const { return m_displayName; }
    QString initials() const { return m_initials; }
    QString avatarSource() const { return m_avatarSource; }
    QString sessionLabel() const { return m_sessionLabel; }

private:
    QString m_userName;
    QString m_displayName;
    QString m_initials;
    QString m_avatarSource;
    QString m_sessionLabel;
};
