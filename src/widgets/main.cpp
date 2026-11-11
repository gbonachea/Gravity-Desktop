#include "clock.h"
#include "iconimageprovider.h"
#include "notificationclient.h"
#include "settingscontroller.h"
#include "shellsurface.h"
#include "storagemonitor.h"
#include "systemhealth.h"
#include "weatherservice.h"

#include <QGuiApplication>
#include <QQmlContext>
#include <QQuickView>
#include <QQuickWindow>

int main(int argc, char *argv[])
{
    QQuickWindow::setDefaultAlphaBuffer(true);

    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("StellarWidgets"));
    app.setApplicationDisplayName(QStringLiteral("Stellar Widgets"));
    app.setOrganizationName(QStringLiteral("Stellar"));
    app.setOrganizationDomain(QStringLiteral("stellar.shell"));
    app.setDesktopFileName(QStringLiteral("stellar-widgets"));

    QQuickView view;
    view.setColor(Qt::transparent);
    view.setFlags(Qt::FramelessWindowHint);
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.resize(Stellar::WidgetsWidth, Stellar::WidgetsHeight);
    view.setTitle(QStringLiteral("Stellar Widgets"));

    view.engine()->addImageProvider(QStringLiteral("icon"), new IconImageProvider);

    Clock clock;
    StorageMonitor storage;
    SettingsController settings;
    WeatherService weather(&settings);
    SystemHealth health;
    NotificationClient notifications;

    QQmlContext *context = view.rootContext();
    context->setContextProperty(QStringLiteral("clock"), &clock);
    context->setContextProperty(QStringLiteral("storage"), &storage);
    context->setContextProperty(QStringLiteral("weather"), &weather);
    context->setContextProperty(QStringLiteral("health"), &health);
    context->setContextProperty(QStringLiteral("settings"), &settings);
    context->setContextProperty(QStringLiteral("notifications"), &notifications);

    view.setSource(QUrl(QStringLiteral("qrc:/qml/widgets/Main.qml")));
    if (view.status() == QQuickView::Error) {
        return 1;
    }

    Stellar::applyLayerSurface(&view, Stellar::SurfaceRole::Widgets);
    view.show();

    return app.exec();
}
