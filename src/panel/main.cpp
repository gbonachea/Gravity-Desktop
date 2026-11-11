#include "clock.h"
#include "iconimageprovider.h"
#include "menuipc.h"
#include "notificationclient.h"
#include "settingscontroller.h"
#include "shellsurface.h"
#include "systemmanager.h"
#include "systemstatus.h"
#include "toplevelmodel.h"

#include <QGuiApplication>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickView>
#include <QQuickWindow>
#include <QTimer>

#include <QScreen>

class PopupController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentType READ currentType NOTIFY currentTypeChanged)
public:
    explicit PopupController(QQuickView *view, QQuickView *catcher, QObject *parent = nullptr)
        : QObject(parent), m_view(view), m_catcher(catcher) {}

    QString currentType() const { return m_type; }

    Q_INVOKABLE void show(const QString &type) {
        if (m_type == type && m_view && m_view->isVisible()) {
            close();
            return;
        }
        m_type = type;
        emit currentTypeChanged();
        if (m_catcher) m_catcher->show();
        if (m_view) m_view->show();
    }

    Q_INVOKABLE void close() {
        m_type.clear();
        emit currentTypeChanged();
        if (m_view) m_view->hide();
        if (m_catcher) m_catcher->hide();
    }

signals:
    void currentTypeChanged();

private:
    QQuickView *m_view = nullptr;
    QQuickView *m_catcher = nullptr;
    QString m_type;
};

class ToastController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap notification READ notification NOTIFY notificationChanged)
public:
    explicit ToastController(QQuickView *view, QObject *parent = nullptr)
        : QObject(parent), m_view(view)
    {
        m_timer.setSingleShot(true);
        m_timer.setInterval(7000);
        connect(&m_timer, &QTimer::timeout, this, &ToastController::hide);
    }

    QVariantMap notification() const { return m_notification; }

    Q_INVOKABLE void show(const QVariantMap &notification)
    {
        m_notification = notification;
        emit notificationChanged();
        if (m_view) {
            if (QObject *root = m_view->rootObject()) {
                root->setProperty("notification", notification);
                QMetaObject::invokeMethod(root, "show",
                                          Q_ARG(QVariant, QVariant::fromValue(notification)));
            }
            m_view->show();
        }
        m_timer.start();
    }

    Q_INVOKABLE void hide()
    {
        m_timer.stop();
        if (m_view) {
            m_view->hide();
        }
    }

signals:
    void notificationChanged();

private:
    QQuickView *m_view = nullptr;
    QVariantMap m_notification;
    QTimer m_timer;
};

int main(int argc, char *argv[])
{
    QQuickWindow::setDefaultAlphaBuffer(true);

    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("StellarPanel"));
    app.setApplicationDisplayName(QStringLiteral("Stellar Panel"));
    app.setOrganizationName(QStringLiteral("Stellar"));
    app.setOrganizationDomain(QStringLiteral("stellar.shell"));
    app.setDesktopFileName(QStringLiteral("stellar-panel"));

    SettingsController settings;

    QQuickView view;
    view.setColor(Qt::transparent);
    view.setFlags(Qt::FramelessWindowHint);
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.resize(1920, settings.panelHeight());
    view.setTitle(QStringLiteral("Stellar Panel"));

    QQuickView popupCatcher;
    popupCatcher.setColor(Qt::transparent);
    popupCatcher.setFlags(Qt::FramelessWindowHint);
    popupCatcher.setResizeMode(QQuickView::SizeRootObjectToView);
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        popupCatcher.resize(screen->geometry().size());
    }
    popupCatcher.setTitle(QStringLiteral("Stellar Popup Catcher"));

    QQuickView popupView;
    popupView.setColor(Qt::transparent);
    popupView.setFlags(Qt::FramelessWindowHint);
    popupView.setResizeMode(QQuickView::SizeRootObjectToView);
    popupView.resize(Stellar::PopupWidth, Stellar::PopupHeight);
    popupView.setTitle(QStringLiteral("Stellar Popup"));

    QQuickView toastView;
    toastView.setColor(Qt::transparent);
    toastView.setFlags(Qt::FramelessWindowHint);
    toastView.setResizeMode(QQuickView::SizeRootObjectToView);
    toastView.resize(Stellar::ToastWidth, Stellar::ToastHeight);
    toastView.setTitle(QStringLiteral("Stellar Notification"));

    view.engine()->addImageProvider(QStringLiteral("icon"), new IconImageProvider);
    popupView.engine()->addImageProvider(QStringLiteral("icon"), new IconImageProvider);
    toastView.engine()->addImageProvider(QStringLiteral("icon"), new IconImageProvider);

    ToplevelModel toplevels;
    Clock clock;
    SystemStatus systemStatus;
    SystemManager system;
    MenuClient menuClient;
    NotificationClient notificationClient;
    PopupController popupController(&popupView, &popupCatcher);
    ToastController toastController(&toastView);

    QQmlContext *context = view.rootContext();
    context->setContextProperty(QStringLiteral("toplevels"), &toplevels);
    context->setContextProperty(QStringLiteral("clock"), &clock);
    context->setContextProperty(QStringLiteral("tray"), &systemStatus);
    context->setContextProperty(QStringLiteral("system"), &system);
    context->setContextProperty(QStringLiteral("settings"), &settings);
    context->setContextProperty(QStringLiteral("menuClient"), &menuClient);
    context->setContextProperty(QStringLiteral("popup"), &popupController);
    context->setContextProperty(QStringLiteral("notifications"), &notificationClient);

    QQmlContext *popupContext = popupView.rootContext();
    popupContext->setContextProperty(QStringLiteral("clock"), &clock);
    popupContext->setContextProperty(QStringLiteral("tray"), &systemStatus);
    popupContext->setContextProperty(QStringLiteral("system"), &system);
    popupContext->setContextProperty(QStringLiteral("popup"), &popupController);
    popupContext->setContextProperty(QStringLiteral("notifications"), &notificationClient);

    toastView.rootContext()->setContextProperty(QStringLiteral("toast"), &toastController);

    QObject::connect(&notificationClient, &NotificationClient::notificationAdded,
                     &toastController, [&settings, &toastController](const QVariantMap &item) {
                         settings.reloadSettings();
                         if (settings.notificationsMuted()
                             || !settings.notificationsPopup()) {
                             return;
                         }
                         toastController.show(item);
                     });

    popupCatcher.rootContext()->setContextProperty(QStringLiteral("popup"), &popupController);

    view.setSource(QUrl(QStringLiteral("qrc:/qml/panel/Main.qml")));
    if (view.status() == QQuickView::Error) {
        return 1;
    }

    popupView.setSource(QUrl(QStringLiteral("qrc:/qml/panel/TrayPopup.qml")));
    if (popupView.status() == QQuickView::Error) {
        return 1;
    }

    popupCatcher.setSource(QUrl(QStringLiteral("qrc:/qml/panel/PopupCatcher.qml")));
    if (popupCatcher.status() == QQuickView::Error) {
        return 1;
    }

    toastView.setSource(QUrl(QStringLiteral("qrc:/qml/panel/NotificationToast.qml")));
    if (toastView.status() == QQuickView::Error) {
        return 1;
    }

    Stellar::applyLayerSurface(&view, Stellar::SurfaceRole::Panel, nullptr,
                               settings.panelPosition() == QLatin1String("top"));
    Stellar::applyLayerSurface(&popupCatcher, Stellar::SurfaceRole::PopupCatcher);
    Stellar::applyLayerSurface(&popupView, Stellar::SurfaceRole::Popup);
    Stellar::applyLayerSurface(&toastView, Stellar::SurfaceRole::NotificationToast);

    popupView.hide();
    popupCatcher.hide();
    toastView.hide();

    view.show();

    return app.exec();
}

#include "main.moc"