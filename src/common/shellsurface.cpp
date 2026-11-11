#include "shellsurface.h"

#include <QGuiApplication>
#include <QScreen>
#include <QTimer>

#if STELLAR_HAVE_LAYERSHELLQT
#include <LayerShellQt/Window>
#endif

namespace Stellar
{

namespace
{

void setupFallbackGeometry(QWindow *window, SurfaceRole role, QScreen *screen,
                           bool panelAtTop = false)
{
    if (!window) {
        return;
    }

    QScreen *target = screen ? screen : window->screen();
    if (!target) {
        target = QGuiApplication::primaryScreen();
    }
    if (!target) {
        return;
    }

    const QRect available = target->availableGeometry();
    switch (role) {
    case SurfaceRole::Background:
        window->setGeometry(target->geometry());
        break;
    case SurfaceRole::Panel:
        window->setGeometry(available.x() + PanelMargin,
                            panelAtTop
                                ? available.y() + PanelMargin
                                : available.y() + available.height() - window->height() - PanelMargin,
                            available.width() - (PanelMargin * 2),
                            window->height());
        break;
    case SurfaceRole::Menu:
        window->setGeometry(available.x() + PanelMargin,
                            available.y() + available.height() - MenuHeight - MenuBottomMargin,
                            MenuWidth,
                            MenuHeight);
        break;
    case SurfaceRole::MenuCatcher:
        window->setGeometry(available.x(), available.y(),
                            available.width(),
                            available.height() - (PanelHeight + PanelMargin * 2));
        break;
    case SurfaceRole::Widgets:
        window->setGeometry(available.x() + available.width() - WidgetsWidth - 24,
                            available.y() + 24,
                            WidgetsWidth,
                            WidgetsHeight);
        break;
    case SurfaceRole::Popup:
        window->setGeometry(available.x() + available.width() - PopupWidth - 24,
                            available.y() + available.height() - PopupHeight - PanelHeight - PanelMargin - 8,
                            PopupWidth,
                            PopupHeight);
        break;
    case SurfaceRole::PopupCatcher:
        window->setGeometry(available);
        break;
    case SurfaceRole::Lock:
        window->setGeometry(target->geometry());
        break;
    case SurfaceRole::NotificationToast:
        window->setGeometry(available.x() + available.width() - ToastWidth - 24,
                            available.y() + available.height() - ToastHeight
                                - PanelHeight - PanelMargin - 8,
                            ToastWidth, ToastHeight);
        break;
    }
}

} // namespace

bool applyLayerSurface(QWindow *window, SurfaceRole role, QScreen *screen,
                       bool panelAtTop)
{
    if (!window) {
        return false;
    }

    const bool isWayland = QGuiApplication::platformName().startsWith(QLatin1String("wayland"));

#if STELLAR_HAVE_LAYERSHELLQT
    if (isWayland) {
        auto *layerWindow = LayerShellQt::Window::get(window);
        if (layerWindow) {
            using LS = LayerShellQt::Window;

            if (screen) {
                layerWindow->setScreen(screen);
            }

            layerWindow->setDesiredSize(window->size());
            layerWindow->setCloseOnDismissed(role != SurfaceRole::MenuCatcher);
            layerWindow->setActivateOnShow(role == SurfaceRole::Menu);
            layerWindow->setExclusiveZone(0);

            switch (role) {
            case SurfaceRole::Background:
                layerWindow->setScope(QStringLiteral("stellar-background"));
                layerWindow->setLayer(LS::LayerBackground);
                layerWindow->setAnchors(LS::Anchors(LS::AnchorTop | LS::AnchorBottom
                                                    | LS::AnchorLeft | LS::AnchorRight));
                layerWindow->setMargins(QMargins(0, 0, 0, 0));
                layerWindow->setKeyboardInteractivity(LS::KeyboardInteractivityNone);
                break;

            case SurfaceRole::Panel:
                layerWindow->setScope(QStringLiteral("stellar-panel"));
                layerWindow->setLayer(LS::LayerTop);
                if (panelAtTop) {
                    layerWindow->setAnchors(LS::Anchors(LS::AnchorLeft | LS::AnchorRight
                                                        | LS::AnchorTop));
                    layerWindow->setMargins(QMargins(PanelMargin, PanelMargin,
                                                     PanelMargin, 0));
                } else {
                    layerWindow->setAnchors(LS::Anchors(LS::AnchorLeft | LS::AnchorRight
                                                        | LS::AnchorBottom));
                    layerWindow->setMargins(QMargins(PanelMargin, 0, PanelMargin, PanelMargin));
                }
                layerWindow->setKeyboardInteractivity(LS::KeyboardInteractivityNone);
                break;

            case SurfaceRole::Menu:
                layerWindow->setScope(QStringLiteral("stellar-menu"));
                layerWindow->setLayer(LS::LayerOverlay);
                layerWindow->setAnchors(LS::Anchors(LS::AnchorLeft | LS::AnchorBottom));
                layerWindow->setMargins(QMargins(PanelMargin, 0, 0, MenuBottomMargin));
                layerWindow->setKeyboardInteractivity(LS::KeyboardInteractivityOnDemand);
                break;

            case SurfaceRole::MenuCatcher:
                // Transparent input surface covering everything except the panel
                // strip at the bottom, so clicks on the panel (including the
                // Start button) reach the panel instead of being swallowed.
                layerWindow->setScope(QStringLiteral("stellar-menu-catcher"));
                layerWindow->setLayer(LS::LayerTop);
                layerWindow->setAnchors(LS::Anchors(LS::AnchorTop | LS::AnchorLeft
                                                    | LS::AnchorRight));
                layerWindow->setMargins(QMargins(0, 0, 0, 0));
                layerWindow->setKeyboardInteractivity(LS::KeyboardInteractivityNone);
                break;

            case SurfaceRole::Widgets:
                layerWindow->setScope(QStringLiteral("stellar-widgets"));
                layerWindow->setLayer(LS::LayerBottom);
                layerWindow->setAnchors(LS::Anchors(LS::AnchorTop | LS::AnchorRight));
                layerWindow->setMargins(QMargins(0, 24, 24, 0));
                layerWindow->setKeyboardInteractivity(LS::KeyboardInteractivityNone);
                break;

            case SurfaceRole::Popup:
                layerWindow->setScope(QStringLiteral("stellar-popup"));
                layerWindow->setLayer(LS::LayerTop);
                layerWindow->setAnchors(LS::Anchors(LS::AnchorBottom | LS::AnchorRight));
                layerWindow->setMargins(QMargins(0, 0, 24, PanelHeight + PanelMargin + 8));
                layerWindow->setKeyboardInteractivity(LS::KeyboardInteractivityOnDemand);
                break;

            case SurfaceRole::PopupCatcher:
                layerWindow->setScope(QStringLiteral("stellar-popup-catcher"));
                layerWindow->setLayer(LS::LayerTop);
                layerWindow->setAnchors(LS::Anchors(LS::AnchorTop | LS::AnchorBottom
                                                    | LS::AnchorLeft | LS::AnchorRight));
                layerWindow->setMargins(QMargins(0, 0, 0, 0));
                layerWindow->setKeyboardInteractivity(LS::KeyboardInteractivityNone);
                break;

            case SurfaceRole::Lock:
                layerWindow->setScope(QStringLiteral("stellar-lock"));
                layerWindow->setLayer(LS::LayerOverlay);
                layerWindow->setAnchors(LS::Anchors(LS::AnchorTop | LS::AnchorBottom
                                                    | LS::AnchorLeft | LS::AnchorRight));
                layerWindow->setMargins(QMargins(0, 0, 0, 0));
                layerWindow->setExclusiveZone(-1);
                layerWindow->setKeyboardInteractivity(LS::KeyboardInteractivityExclusive);
                layerWindow->setActivateOnShow(true);
                layerWindow->setCloseOnDismissed(false);
                break;

            case SurfaceRole::NotificationToast:
                layerWindow->setScope(QStringLiteral("stellar-notification-toast"));
                layerWindow->setLayer(LS::LayerTop);
                layerWindow->setAnchors(LS::Anchors(LS::AnchorBottom | LS::AnchorRight));
                layerWindow->setMargins(QMargins(0, 0, 24, PanelHeight + PanelMargin + 8));
                layerWindow->setKeyboardInteractivity(LS::KeyboardInteractivityNone);
                break;
            }
            return true;
        }
    }
#endif

    // X11 / offscreen / compositor without layer-shell: keep the shell usable
    // by placing the surfaces like ordinary floating windows.
    setupFallbackGeometry(window, role, screen, panelAtTop);
    return false;
}

} // namespace Stellar
