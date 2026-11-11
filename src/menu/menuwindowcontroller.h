#pragma once

#include <QObject>
#include <QElapsedTimer>

class QQuickView;

/*!
 * Owns visibility and focus semantics for the layer-shell start menu surface.
 *
 * The menu is paired with a full-screen transparent "catcher" surface in the
 * layer below it. Layer-shell surfaces cannot notice clicks outside themselves,
 * so the catcher turns any click outside the menu into a dismiss request.
 */
class MenuWindowController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool visible READ visible NOTIFY visibleChanged)

public:
    explicit MenuWindowController(QQuickView *view, QQuickView *catcher,
                                  QObject *parent = nullptr);

    bool visible() const;

    Q_INVOKABLE void showMenu();
    Q_INVOKABLE void hideMenu();
    Q_INVOKABLE void toggle();

signals:
    void visibleChanged();
    void shown();
    void hidden();

private:
    void setCatcherVisible(bool visible);

    QQuickView *m_view = nullptr;
    QQuickView *m_catcher = nullptr;
    QElapsedTimer m_lastHide;
};
