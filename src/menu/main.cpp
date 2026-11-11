#include "applauncher.h"
#include "desktopentrymodel.h"
#include "iconimageprovider.h"
#include "menuwindowcontroller.h"
#include "menuipc.h"
#include "sessioninfo.h"
#include "settingscontroller.h"
#include "shellsurface.h"
#include "systemactions.h"
#include "systemmanager.h"

#include <QGuiApplication>
#include <QLocalSocket>
#include <QQmlContext>
#include <QQuickView>
#include <QQuickWindow>
#include <QScreen>
#include <QTimer>

int main(int argc, char *argv[])
{
    QQuickWindow::setDefaultAlphaBuffer(true);

    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("StellarMenu"));
    app.setApplicationDisplayName(QStringLiteral("Stellar Menu"));
    app.setOrganizationName(QStringLiteral("Stellar"));
    app.setOrganizationDomain(QStringLiteral("stellar.shell"));
    app.setDesktopFileName(QStringLiteral("stellar-menu"));

    MenuServer server;
    if (!server.listen()) {
        // Another menu is already running: ask it to show itself.
        QLocalSocket socket;
        socket.connectToServer(menuSocketName());
        if (socket.waitForConnected(200)) {
            socket.write("show\n");
            socket.flush();
            socket.waitForBytesWritten(200);
            socket.disconnectFromServer();
        }
        return 0;
    }

    QQuickView view;
    view.setColor(Qt::transparent);
    view.setFlags(Qt::FramelessWindowHint);
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.resize(Stellar::MenuWidth, Stellar::MenuHeight);
    view.setTitle(QStringLiteral("Stellar Menu"));

    view.engine()->addImageProvider(QStringLiteral("icon"), new IconImageProvider);

    QQuickView catcher;
    catcher.setColor(Qt::transparent);
    catcher.setFlags(Qt::FramelessWindowHint);
    catcher.setResizeMode(QQuickView::SizeRootObjectToView);
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        catcher.resize(screen->geometry().width(),
                       screen->geometry().height() - (Stellar::PanelHeight
                                                      + Stellar::PanelMargin * 2));
    }
    catcher.setTitle(QStringLiteral("Stellar Menu Catcher"));

    SettingsController settings;
    DesktopEntryModel applications;
    applications.setMaxEntries(11);
    applications.setFavorites(settings.menuFavorites());
    AppLauncher launcher;
    SystemActions systemActions;
    SystemManager system;
    SessionInfo session;
    MenuWindowController windowController(&view, &catcher);

    QObject::connect(&settings, &SettingsController::menuFavoritesChanged, &applications,
                     [&settings, &applications] {
                         applications.setFavorites(settings.menuFavorites());
                     });

    // El Centro de configuración es otro proceso: al abrir el menú se relee el
    // archivo para reflejar los favoritos recién añadidos o quitados.
    QObject::connect(&windowController, &MenuWindowController::shown, &applications,
                     [&settings, &applications] {
                         settings.reloadSettings();
                         applications.setFavorites(settings.menuFavorites());
                     });

    QQmlContext *context = view.rootContext();
    context->setContextProperty(QStringLiteral("apps"), &applications);
    context->setContextProperty(QStringLiteral("launcher"), &launcher);
    context->setContextProperty(QStringLiteral("systemActions"), &systemActions);
    context->setContextProperty(QStringLiteral("system"), &system);
    context->setContextProperty(QStringLiteral("settings"), &settings);
    context->setContextProperty(QStringLiteral("session"), &session);
    context->setContextProperty(QStringLiteral("menuWindow"), &windowController);

    catcher.rootContext()->setContextProperty(QStringLiteral("menuWindow"),
                                             &windowController);

    view.setSource(QUrl(QStringLiteral("qrc:/qml/menu/Main.qml")));
    if (view.status() == QQuickView::Error) {
        return 1;
    }

    catcher.setSource(QUrl(QStringLiteral("qrc:/qml/menu/Catcher.qml")));
    if (catcher.status() == QQuickView::Error) {
        return 1;
    }

    Stellar::applyLayerSurface(&view, Stellar::SurfaceRole::Menu);
    Stellar::applyLayerSurface(&catcher, Stellar::SurfaceRole::MenuCatcher);

    QObject::connect(&server, &MenuServer::toggleRequested, &windowController,
                     &MenuWindowController::toggle);
    QObject::connect(&server, &MenuServer::showRequested, &windowController,
                     &MenuWindowController::showMenu);
    QObject::connect(&server, &MenuServer::hideRequested, &windowController,
                     &MenuWindowController::hideMenu);
    QObject::connect(&server, &MenuServer::quitRequested, &app, &QGuiApplication::quit);

    // Cerrar el menú cuando pierde el foco (p. ej. al hacer clic en otra
    // ventana). Se espera un poco para no cerrarlo durante la activación
    // inicial tras mostrarlo.
    QObject::connect(&view, &QQuickWindow::activeChanged, &windowController,
                     [&view, &windowController] {
                         if (view.isActive() || !windowController.visible()) {
                             return;
                         }
                         QTimer::singleShot(150, &windowController, [&view, &windowController] {
                             if (!view.isActive() && windowController.visible()) {
                                 windowController.hideMenu();
                             }
                         });
                     });

    // The surface stays hidden until the panel sends "toggle" or "show".
    return app.exec();
}
