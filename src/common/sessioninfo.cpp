#include "sessioninfo.h"

#include <QDir>
#include <QFileInfo>
#include <QUrl>

#include <pwd.h>
#include <unistd.h>

namespace
{

QString initialsFor(const QString &name)
{
    const QStringList words = name.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    QString initials;
    for (const QString &word : words) {
        if (initials.size() >= 2) {
            break;
        }
        if (!word.isEmpty() && word.at(0).isLetter()) {
            initials.append(word.at(0).toUpper());
        }
    }
    return initials.isEmpty() ? QStringLiteral("XP") : initials;
}

} // namespace

SessionInfo::SessionInfo(QObject *parent)
    : QObject(parent)
{
    const QString fallbackUser = qEnvironmentVariable("USER", QStringLiteral("usuario"));
    m_userName = fallbackUser;
    QString realName;

    if (const passwd *pw = getpwuid(getuid())) {
        m_userName = QString::fromLocal8Bit(pw->pw_name);
        const QString gecos = QString::fromLocal8Bit(pw->pw_gecos);
        realName = gecos.section(QLatin1Char(','), 0, 0).trimmed();
    }

    m_displayName = realName.isEmpty() ? QStringLiteral("Usuario XP") : realName;
    m_initials = initialsFor(m_displayName);

    const QString avatarPath = QDir::homePath() + QStringLiteral("/.face");
    if (QFileInfo::exists(avatarPath)) {
        m_avatarSource = QUrl::fromLocalFile(avatarPath).toString();
    }

    m_sessionLabel = QStringLiteral("Sesión local · Wayland");
}
