#pragma once

#include <QAbstractListModel>
#include <QFileSystemWatcher>
#include <QHash>
#include <QString>
#include <QTimer>
#include <QVariantMap>
#include <QVector>

class SettingsController;

/*!
 * Modelo de los iconos que se dibujan sobre el fondo de pantalla.
 *
 * Combina los accesos especiales (Equipo, Usuario, Papelera) que se activan
 * desde el Centro de configuración con el contenido de la carpeta Escritorio
 * (~/Escritorio o el directorio XDG configurado). El escritorio del shell no
 * depende de un gestor de archivos: stellar-background pinta estos iconos en
 * su superficie de capa Background.
 */
class DesktopIconModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        IconRole,
        KindRole,
        PathRole,
        RemovableRole,
        RemoveKindRole,
    };

    explicit DesktopIconModel(SettingsController *settings, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return int(m_items.size()); }

    Q_INVOKABLE void reload();
    Q_INVOKABLE bool open(int row);
    Q_INVOKABLE bool openTerminal(int row);
    Q_INVOKABLE bool remove(int row);
    Q_INVOKABLE bool createFolder();
    Q_INVOKABLE bool openDesktopFolder();
    Q_INVOKABLE QVariantMap itemAt(int row) const;

signals:
    void countChanged();
    void actionFailed(const QString &message);

private:
    struct Item {
        QString name;
        QString icon;
        QString kind;
        QString path;
        QString removeKind;
        bool removable = false;
    };

    void rebuild();
    void scheduleReload();
    QString desktopDir() const;
    bool removeFile(const QString &path);

    SettingsController *m_settings = nullptr;
    QVector<Item> m_items;
    QFileSystemWatcher m_watcher;
    QTimer m_reloadTimer;
};
