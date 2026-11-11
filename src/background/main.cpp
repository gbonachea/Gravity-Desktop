#include "desktopiconmodel.h"
#include "iconimageprovider.h"
#include "settingscontroller.h"
#include "shellsurface.h"
#include "systemactions.h"
#include "wallpapersource.h"

#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickView>
#include <QQuickWindow>
#include <QScreen>
#include <QSettings>

int main(int argc, char *argv[])
{
    QQuickWindow::setDefaultAlphaBuffer(true);

    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("StellarBackground"));
    app.setApplicationDisplayName(QStringLiteral("Stellar Background"));
    app.setOrganizationName(QStringLiteral("Stellar"));
    app.setOrganizationDomain(QStringLiteral("stellar.shell"));
    app.setDesktopFileName(QStringLiteral("stellar-background"));

    // El escritorio comparte los ajustes del shell: tema de iconos, accesos
    // especiales y carpeta de escritorio.
    SettingsController settings;
    DesktopIconModel desktopIcons(&settings);
    SystemActions systemActions;

    QQmlEngine engine;
    WallpaperSource wallpaper;
    engine.addImageProvider(QStringLiteral("icon"), new IconImageProvider);
    engine.rootContext()->setContextProperty(QStringLiteral("wallpaper"), &wallpaper);
    engine.rootContext()->setContextProperty(QStringLiteral("settings"), &settings);
    engine.rootContext()->setContextProperty(QStringLiteral("desktopIcons"), &desktopIcons);
    engine.rootContext()->setContextProperty(QStringLiteral("systemActions"), &systemActions);

    const QList<QScreen *> screens = QGuiApplication::screens();
    if (screens.isEmpty()) {
        return 1;
    }

    QList<QQuickView *> views;
    views.reserve(screens.size());

    for (QScreen *screen : screens) {
        auto *view = new QQuickView(&engine, nullptr);
        view->setScreen(screen);
        view->setColor(Qt::black);
        view->setFlags(Qt::FramelessWindowHint);
        view->setResizeMode(QQuickView::SizeRootObjectToView);
        view->resize(screen->geometry().size());
        view->setTitle(QStringLiteral("Stellar Background"));

        view->setSource(QUrl(QStringLiteral("qrc:/qml/background/Main.qml")));
        if (view->status() == QQuickView::Error) {
            return 1;
        }

        Stellar::applyLayerSurface(view, Stellar::SurfaceRole::Background, screen);
        view->show();
        views.append(view);
    }

    return app.exec();
}
