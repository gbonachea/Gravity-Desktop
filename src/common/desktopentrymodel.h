#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QVector>

struct DesktopEntry
{
    QString id;
    QString name;
    QString genericName;
    QString comment;
    QString icon;
    QString exec;
    QString tryExec;
    QString path;
    QString sourceFile;
    QString startupWmClass;
    QString fileName;
    QString type;
    QStringList categories;
    bool terminal = false;
    bool noDisplay = false;
    bool hidden = false;
};

// Utilidades compartidas por el índice del menú y por los accesos del
// escritorio (que pueden vivir fuera de los directorios XDG).
DesktopEntry parseDesktopFile(const QString &filePath);
bool isValidApplication(const DesktopEntry &entry);

/*!
 * Small index over the XDG application menus. It is intentionally process
 * local: every shell component parses the .desktop files once at startup.
 */
class DesktopEntryIndex
{
public:
    static DesktopEntryIndex &instance();

    const QVector<DesktopEntry> &entries() const { return m_entries; }
    QString iconForAppId(const QString &appId) const;
    const DesktopEntry *findByDesktopFile(const QString &desktopFile) const;

    void reload();

private:
    DesktopEntryIndex();
    void load();

    QVector<DesktopEntry> m_entries;
    QHash<QString, QString> m_iconByKey;
    QHash<QString, int> m_indexByFile;
};

/*!
 * Filterable, alphabetically sorted model used by the start menu.
 */
class DesktopEntryModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged)
    Q_PROPERTY(int maxEntries READ maxEntries WRITE setMaxEntries NOTIFY maxEntriesChanged)
    Q_PROPERTY(QStringList favorites READ favorites WRITE setFavorites NOTIFY favoritesChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        IconRole,
        CommentRole,
        DesktopFileRole,
        IdRole,
        CategoriesRole,
    };

    explicit DesktopEntryModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return int(m_visible.size()); }
    QString filterText() const { return m_filterText; }
    void setFilterText(const QString &text);
    int maxEntries() const { return m_maxEntries; }
    void setMaxEntries(int value);
    QStringList favorites() const { return m_favorites; }
    void setFavorites(const QStringList &value);

    Q_INVOKABLE bool launch(int row);
    Q_INVOKABLE bool launchDesktopFile(const QString &desktopFile);
    Q_INVOKABLE void refresh();

signals:
    void countChanged();
    void filterTextChanged();
    void maxEntriesChanged();
    void favoritesChanged();
    void launchFailed(const QString &message);
    void launched(const QString &name);

private:
    void rebuild();
    bool entryMatches(const DesktopEntry &entry) const;

    QVector<DesktopEntry> m_all;
    QVector<DesktopEntry> m_visible;
    QString m_filterText;
    int m_maxEntries = -1;
    QStringList m_favorites;
};
