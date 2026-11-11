#pragma once

#include <QWindow>

class QScreen;

namespace Stellar
{

enum class SurfaceRole {
    Background,
    Panel,
    Menu,
    MenuCatcher,
    Widgets,
    Popup,
    PopupCatcher,
    Lock,
    NotificationToast,
};

inline constexpr int PopupWidth = 460;
inline constexpr int PopupHeight = 570;

inline constexpr int PanelHeight = 64;
inline constexpr int PanelMargin = 12;

inline constexpr int MenuWidth = 680;
inline constexpr int MenuHeight = 600;
inline constexpr int MenuBottomMargin = PanelHeight + PanelMargin + 8;

inline constexpr int WidgetsWidth = 360;
inline constexpr int WidgetsHeight = 520;

inline constexpr int ToastWidth = 400;
inline constexpr int ToastHeight = 176;

/*!
 * Applies wlr-layer-shell properties to a window. Returns true when the
 * layer-shell backend was actually used. On X11/offscreen the window is
 * positioned as a useful floating fallback.
 */
bool applyLayerSurface(QWindow *window, SurfaceRole role, QScreen *screen = nullptr,
                       bool panelAtTop = false);

} // namespace Stellar
