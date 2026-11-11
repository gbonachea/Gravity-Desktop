#pragma once

#include <QObject>
#include <QVariantList>
#include <QVector>

/*!
 * Lectura y cambio de las aplicaciones predeterminadas del sistema mediante
 * las asociaciones XDG (xdg-mime / gio mime) y xdg-settings para el navegador.
 */
class DefaultAppsController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList categories READ categories NOTIFY categoriesChanged)

public:
    explicit DefaultAppsController(QObject *parent = nullptr);

    QVariantList categories() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool setDefault(const QString &categoryId, const QString &desktopFile);
    Q_INVOKABLE void openSystemSettings();

signals:
    void categoriesChanged();
    void errorOccurred(const QString &message);

private:
    struct Category {
        QString id;
        QString label;
        QStringList mimeTypes;
        QStringList appCategories;
        bool webBrowser = false;
    };

    QVector<Category> definitions() const;
    QString queryDefault(const Category &category) const;
    QVariantList choicesFor(const Category &category) const;
    bool setMimeDefault(const QString &desktopFile, const QString &mimeType);
    static QString appName(const QString &desktopFile);
    static QString appIcon(const QString &desktopFile);
};
