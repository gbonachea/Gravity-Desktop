#include "desktopentrymodel.h"

#include "applauncher.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QSet>
#include <QStandardPaths>
#include <QTextStream>

#include <algorithm>

namespace
{

QString unescapeDesktopValue(const QString &value)
{
    QString out;
    out.reserve(value.size());
    for (int i = 0; i < value.size(); ++i) {
        const QChar c = value.at(i);
        if (c != QLatin1Char('\\') || i + 1 >= value.size()) {
            out.append(c);
            continue;
        }
        const QChar next = value.at(++i);
        switch (next.unicode()) {
        case u's':
            out.append(QLatin1Char(' '));
            break;
        case u'n':
            out.append(QLatin1Char('\n'));
            break;
        case u't':
            out.append(QLatin1Char('\t'));
            break;
        case u'r':
            out.append(QLatin1Char('\r'));
            break;
        case u'\\':
            out.append(QLatin1Char('\\'));
            break;
        default:
            out.append(next);
            break;
        }
    }
    return out;
}

QString localizedValue(const QHash<QString, QString> &values, const QString &key)
{
    const QString full = QLocale::system().name(); // e.g. es_ES
    const QString language = full.section(QLatin1Char('_'), 0, 0);

    const QStringList candidates = {
        key + QLatin1Char('[') + full + QLatin1Char(']'),
        key + QLatin1Char('[') + language + QLatin1Char(']'),
        key,
    };

    for (const QString &candidate : candidates) {
        const auto it = values.constFind(candidate);
        if (it != values.constEnd()) {
            return unescapeDesktopValue(it.value());
        }
    }
    return {};
}

} // namespace

DesktopEntry parseDesktopFile(const QString &filePath)
{
    DesktopEntry entry;
    entry.fileName = QFileInfo(filePath).fileName();
    entry.id = QFileInfo(filePath).completeBaseName();
    entry.sourceFile = QFileInfo(filePath).absoluteFilePath();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return entry;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);

    bool inDesktopEntry = false;
    QHash<QString, QString> values;
    while (!stream.atEnd()) {
        const QString line = stream.readLine();
        if (line.startsWith(QLatin1Char('#')) || line.trimmed().isEmpty()) {
            continue;
        }
        if (line.startsWith(QLatin1Char('['))) {
            inDesktopEntry = line.trimmed() == QLatin1String("[Desktop Entry]");
            continue;
        }
        if (!inDesktopEntry) {
            continue;
        }
        const int equals = line.indexOf(QLatin1Char('='));
        if (equals <= 0) {
            continue;
        }
        values.insert(line.left(equals).trimmed(), line.mid(equals + 1));
    }

    entry.name = localizedValue(values, QStringLiteral("Name"));
    entry.genericName = localizedValue(values, QStringLiteral("GenericName"));
    entry.comment = localizedValue(values, QStringLiteral("Comment"));
    entry.icon = localizedValue(values, QStringLiteral("Icon"));
    entry.exec = localizedValue(values, QStringLiteral("Exec"));
    entry.tryExec = localizedValue(values, QStringLiteral("TryExec"));
    entry.path = localizedValue(values, QStringLiteral("Path"));
    entry.startupWmClass = localizedValue(values, QStringLiteral("StartupWMClass"));
    entry.type = values.value(QStringLiteral("Type"));
    entry.terminal = values.value(QStringLiteral("Terminal")).compare(QLatin1String("true"),
                                                                     Qt::CaseInsensitive)
        == 0;
    entry.noDisplay = values.value(QStringLiteral("NoDisplay")).compare(QLatin1String("true"),
                                                                       Qt::CaseInsensitive)
        == 0;
    entry.hidden = values.value(QStringLiteral("Hidden")).compare(QLatin1String("true"),
                                                                  Qt::CaseInsensitive)
        == 0;

    const QString categories = values.value(QStringLiteral("Categories"));
    for (const QString &category : categories.split(QLatin1Char(';'), Qt::SkipEmptyParts)) {
        entry.categories.append(category.trimmed());
    }

    return entry;
}

bool isValidApplication(const DesktopEntry &entry)
{
    if (entry.name.isEmpty() || entry.exec.isEmpty()) {
        return false;
    }
    if (!entry.type.isEmpty() && entry.type != QLatin1String("Application")) {
        return false;
    }
    if (entry.noDisplay || entry.hidden) {
        return false;
    }
    if (!entry.tryExec.isEmpty()
        && QStandardPaths::findExecutable(entry.tryExec).isEmpty()
        && !QFileInfo::exists(entry.tryExec)) {
        return false;
    }
    return true;
}

DesktopEntryIndex::DesktopEntryIndex()
{
    load();
}

DesktopEntryIndex &DesktopEntryIndex::instance()
{
    static DesktopEntryIndex index;
    return index;
}

void DesktopEntryIndex::reload()
{
    m_entries.clear();
    m_iconByKey.clear();
    m_indexByFile.clear();
    load();
}

void DesktopEntryIndex::load()
{
    const QStringList directories =
        QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);

    QSet<QString> seenIds;
    QStringList files;

    for (const QString &directory : directories) {
        const QDir dir(directory);
        if (!dir.exists()) {
            continue;
        }
        const QStringList entries = dir.entryList({QStringLiteral("*.desktop")}, QDir::Files,
                                                  QDir::Name);
        for (const QString &fileName : entries) {
            files.append(dir.filePath(fileName));
        }
    }

    for (const QString &filePath : files) {
        DesktopEntry entry = parseDesktopFile(filePath);
        if (!isValidApplication(entry)) {
            continue;
        }

        // XDG lookup order means the first declaration wins (user override).
        if (seenIds.contains(entry.id)) {
            continue;
        }
        seenIds.insert(entry.id);

        m_entries.append(entry);

        const QString icon = entry.icon.isEmpty()
            ? QStringLiteral("application-x-executable")
            : entry.icon;

        m_iconByKey.insert(entry.id.toLower(), icon);
        m_iconByKey.insert(entry.fileName.toLower(), icon);
        if (!entry.startupWmClass.isEmpty()) {
            m_iconByKey.insert(entry.startupWmClass.toLower(), icon);
        }
    }

    std::sort(m_entries.begin(), m_entries.end(), [](const DesktopEntry &a, const DesktopEntry &b) {
        return QString::localeAwareCompare(a.name, b.name) < 0;
    });

    // Rebuild the file index after sorting.
    m_indexByFile.clear();
    for (int i = 0; i < m_entries.size(); ++i) {
        const DesktopEntry &entry = m_entries.at(i);
        m_indexByFile.insert(entry.fileName, i);
        m_indexByFile.insert(entry.id, i);
        m_indexByFile.insert(entry.id + QStringLiteral(".desktop"), i);
        m_indexByFile.insert(entry.sourceFile, i);
    }
}

QString DesktopEntryIndex::iconForAppId(const QString &appId) const
{
    if (appId.isEmpty()) {
        return QStringLiteral("application-x-executable");
    }

    QString key = appId.toLower();
    if (key.startsWith(QLatin1Char('/'))) {
        key = QFileInfo(key).fileName();
    }
    if (key.endsWith(QLatin1String(".desktop"))) {
        key.chop(8);
    }

    auto lookup = [this](const QString &candidate) -> QString {
        return m_iconByKey.value(candidate.toLower(), QString());
    };

    QString icon = lookup(key);
    if (!icon.isEmpty()) {
        return icon;
    }

    // org.kde.konsole -> kde.konsole -> konsole
    const QStringList parts = key.split(QLatin1Char('.'));
    for (int i = 1; i < parts.size(); ++i) {
        icon = lookup(parts.mid(i).join(QLatin1Char('.')));
        if (!icon.isEmpty()) {
            return icon;
        }
    }

    return QStringLiteral("application-x-executable");
}

const DesktopEntry *DesktopEntryIndex::findByDesktopFile(const QString &desktopFile) const
{
    if (desktopFile.isEmpty()) {
        return nullptr;
    }

    const QFileInfo info(desktopFile);
    auto it = m_indexByFile.constFind(info.absoluteFilePath());
    if (it == m_indexByFile.constEnd()) {
        it = m_indexByFile.constFind(info.fileName());
    }
    if (it == m_indexByFile.constEnd()) {
        it = m_indexByFile.constFind(info.completeBaseName());
    }
    if (it != m_indexByFile.constEnd() && it.value() >= 0 && it.value() < m_entries.size()) {
        return &m_entries.at(it.value());
    }
    return nullptr;
}

DesktopEntryModel::DesktopEntryModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_all = DesktopEntryIndex::instance().entries();
    rebuild();
}

int DesktopEntryModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return int(m_visible.size());
}

QVariant DesktopEntryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visible.size()) {
        return {};
    }

    const DesktopEntry &entry = m_visible.at(index.row());
    switch (role) {
    case NameRole:
        return entry.name;
    case IconRole:
        return entry.icon.isEmpty() ? QStringLiteral("application-x-executable") : entry.icon;
    case CommentRole:
        return entry.comment.isEmpty() ? entry.genericName : entry.comment;
    case DesktopFileRole:
        return entry.fileName;
    case IdRole:
        return entry.id;
    case CategoriesRole:
        return entry.categories;
    default:
        return {};
    }
}

QHash<int, QByteArray> DesktopEntryModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {IconRole, "icon"},
        {CommentRole, "comment"},
        {DesktopFileRole, "desktopFile"},
        {IdRole, "entryId"},
        {CategoriesRole, "categories"},
    };
}

void DesktopEntryModel::setFilterText(const QString &text)
{
    if (m_filterText == text) {
        return;
    }
    m_filterText = text;
    emit filterTextChanged();
    rebuild();
}

void DesktopEntryModel::setMaxEntries(int value)
{
    const int normalized = value <= 0 ? -1 : value;
    if (m_maxEntries == normalized) {
        return;
    }
    m_maxEntries = normalized;
    emit maxEntriesChanged();
    rebuild();
}

bool DesktopEntryModel::entryMatches(const DesktopEntry &entry) const
{
    if (m_filterText.trimmed().isEmpty()) {
        return true;
    }

    const QString needle = m_filterText.trimmed();
    return entry.name.contains(needle, Qt::CaseInsensitive)
        || entry.comment.contains(needle, Qt::CaseInsensitive)
        || entry.genericName.contains(needle, Qt::CaseInsensitive)
        || entry.categories.join(QLatin1Char(' ')).contains(needle, Qt::CaseInsensitive);
}

void DesktopEntryModel::rebuild()
{
    beginResetModel();
    m_visible.clear();

    // Vista compacta: solo los favoritos, en el orden elegido. Si el usuario
    // no tiene favoritos la lista queda vacía a propósito; "Todos los
    // programas" (maxEntries < 0) sigue mostrando el catálogo completo.
    const bool showFavorites =
        m_filterText.trimmed().isEmpty() && m_maxEntries > 0;

    if (showFavorites) {
        // Vista compacta: solo los favoritos, en el orden elegido.
        for (const QString &file : m_favorites) {
            for (const DesktopEntry &entry : std::as_const(m_all)) {
                if (entry.fileName == file) {
                    m_visible.append(entry);
                    break;
                }
            }
        }
    } else {
        m_visible.reserve(m_all.size());
        for (const DesktopEntry &entry : std::as_const(m_all)) {
            if (!entryMatches(entry)) {
                continue;
            }
            if (m_maxEntries > 0 && m_visible.size() >= m_maxEntries) {
                break;
            }
            m_visible.append(entry);
        }
    }

    endResetModel();
    emit countChanged();
}

void DesktopEntryModel::setFavorites(const QStringList &value)
{
    if (value == m_favorites) {
        return;
    }
    m_favorites = value;
    emit favoritesChanged();
    rebuild();
}

bool DesktopEntryModel::launch(int row)
{
    if (row < 0 || row >= m_visible.size()) {
        return false;
    }
    return launchDesktopFile(m_visible.at(row).fileName);
}

bool DesktopEntryModel::launchDesktopFile(const QString &desktopFile)
{
    const DesktopEntry *entry = DesktopEntryIndex::instance().findByDesktopFile(desktopFile);
    if (!entry) {
        emit launchFailed(tr("No se encontró la entrada de escritorio: %1").arg(desktopFile));
        return false;
    }

    // AppLauncher owns the process semantics; using it from here keeps QML
    // simple and guarantees identical behaviour in every surface.
    static AppLauncher launcher;
    const bool ok = launcher.launchDesktopFile(desktopFile);
    if (ok) {
        emit launched(entry->name);
    } else {
        emit launchFailed(tr("No se pudo iniciar %1").arg(entry->name));
    }
    return ok;
}

void DesktopEntryModel::refresh()
{
    DesktopEntryIndex::instance().reload();
    m_all = DesktopEntryIndex::instance().entries();
    rebuild();
}
