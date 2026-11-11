#include "clock.h"
#include "lockcontroller.h"
#include "shellsurface.h"
#include "wallpapersource.h"

#include <QDir>
#include <QGuiApplication>
#include <QLockFile>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickView>
#include <QQuickWindow>
#include <QScreen>
#include <QStandardPaths>

int main(int argc, char *argv[])
{
    QQuickWindow::setDefaultAlphaBuffer(true);

    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("StellarLock"));
    app.setApplicationDisplayName(QStringLiteral("Stellar Lock"));
    app.setOrganizationName(QStringLiteral("Stellar"));
    app.setOrganizationDomain(QStringLiteral("stellar.shell"));
    app.setDesktopFileName(QStringLiteral("stellar-lock"));

    // Una sola instancia del bloqueador por sesión. Si un intento anterior
    // terminó de forma abrupta, se descarta su bloqueo obsoleto.
    QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (runtimeDir.isEmpty()) {
        runtimeDir = QDir::tempPath();
    }
    QLockFile instanceLock(runtimeDir + QStringLiteral("/stellar-lock.lock"));
    instanceLock.setStaleLockTime(30000);
    if (!instanceLock.tryLock(200)) {
        instanceLock.removeStaleLockFile();
        if (!instanceLock.tryLock(200)) {
            return 0;
        }
    }

    WallpaperSource wallpaper;
    Clock clock;
    LockController lock;

    QQmlEngine engine;
    QQmlContext *context = engine.rootContext();
    context->setContextProperty(QStringLiteral("wallpaper"), &wallpaper);
    context->setContextProperty(QStringLiteral("clock"), &clock);
    context->setContextProperty(QStringLiteral("lock"), &lock);

    QObject::connect(&lock, &LockController::authenticated, &app,
                     &QGuiApplication::quit);

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
        view->setTitle(QStringLiteral("Stellar Lock"));

        view->setSource(QUrl(QStringLiteral("qrc:/qml/lock/Main.qml")));
        if (view->status() == QQuickView::Error) {
            return 1;
        }

        Stellar::applyLayerSurface(view, Stellar::SurfaceRole::Lock, screen);
        view->show();
        view->requestActivate();
        views.append(view);
    }

    return app.exec();
}
