#include "menuwindowcontroller.h"

#include <QQuickView>
#include <QTimer>

MenuWindowController::MenuWindowController(QQuickView *view, QQuickView *catcher,
                                           QObject *parent)
    : QObject(parent)
    , m_view(view)
    , m_catcher(catcher)
{
}

bool MenuWindowController::visible() const
{
    return m_view && m_view->isVisible();
}

void MenuWindowController::setCatcherVisible(bool visible)
{
    if (!m_catcher) {
        return;
    }

    if (visible) {
        // Show the catcher first: the menu lives in the Overlay layer, so it
        // always stays above this Top-layer input surface.
        m_catcher->show();
    } else {
        m_catcher->hide();
    }
}

void MenuWindowController::showMenu()
{
    if (!m_view || m_view->isVisible()) {
        return;
    }

    setCatcherVisible(true);
    m_view->show();
    m_view->requestActivate();
    QTimer::singleShot(60, m_view, [this] {
        if (m_view && m_view->isVisible()) {
            m_view->requestActivate();
        }
    });

    emit shown();
    emit visibleChanged();
}

void MenuWindowController::hideMenu()
{
    if (!m_view) {
        setCatcherVisible(false);
        return;
    }

    if (m_view->isVisible()) {
        m_view->hide();
        m_lastHide.start();
        emit hidden();
        emit visibleChanged();
    }

    setCatcherVisible(false);
}

void MenuWindowController::toggle()
{
    if (visible()) {
        hideMenu();
    } else if (m_lastHide.isValid() && m_lastHide.elapsed() < 700) {
        // Evita reabrir inmediatamente tras un cierre (p. ej. clic en el
        // botón Inicio justo después de que el catcher cerrara el menú).
        return;
    } else {
        showMenu();
    }
}
