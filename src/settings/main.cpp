#include "defaultappscontroller.h"
#include "devicescontroller.h"
#include "iconimageprovider.h"
#include "settingscontroller.h"
#include "systemmanager.h"
#include "systemstatus.h"

#include <QGuiApplication>
#include <QQmlContext>
#include <QQuickView>
#include <QQuickWindow>

int main(int argc, char *argv[])
{
    QQuickWindow::setDefaultAlphaBuffer(true);

    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("StellarSettings"));
    app.setApplicationDisplayName(QStringLiteral("Centro de configuración"));
    app.setOrganizationName(QStringLiteral("Stellar"));
    app.setOrganizationDomain(QStringLiteral("stellar.shell"));
    app.setDesktopFileName(QStringLiteral("stellar-settings"));

    QQuickView view;
    view.setColor(Qt::transparent);
    view.setFlags(Qt::FramelessWindowHint);
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.resize(1040, 700);
    view.setTitle(QStringLiteral("Centro de configuración"));

    view.engine()->addImageProvider(QStringLiteral("icon"), new IconImageProvider);

    SystemManager system;
    SystemStatus systemStatus;
    SettingsController settings;
    DevicesController devices;
    DefaultAppsController defaultApps;

    QQmlContext *context = view.rootContext();
    context->setContextProperty(QStringLiteral("system"), &system);
    context->setContextProperty(QStringLiteral("tray"), &systemStatus);
    context->setContextProperty(QStringLiteral("settings"), &settings);
    context->setContextProperty(QStringLiteral("devices"), &devices);
    context->setContextProperty(QStringLiteral("defaultApps"), &defaultApps);

    view.setSource(QUrl(QStringLiteral("qrc:/qml/settings/Main.qml")));
    if (view.status() == QQuickView::Error) {
        return 1;
    }

    view.show();
    return app.exec();
}