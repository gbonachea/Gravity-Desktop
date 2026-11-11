#pragma once

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QSet>
#include <QString>
#include <QTimer>
#include <QVariant>
#include <QVector>

struct wl_display;
struct wl_event_queue;
struct wl_registry;
struct wl_seat;
struct wl_output;
struct wl_array;
struct zwlr_foreign_toplevel_manager_v1;
struct zwlr_foreign_toplevel_handle_v1;

/*!
 * Taskbar model backed by the wlr-foreign-toplevel-management protocol.
 *
 * The protocol objects live in their own wl_event_queue. Qt's Wayland event
 * loop drains the socket, and a small poll timer dispatches our queue, which
 * keeps the implementation independent from QtWaylandClient private APIs.
 */
class ToplevelModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)

public:
    enum Roles {
        TitleRole = Qt::UserRole + 1,
        AppIdRole,
        IconRole,
        ActiveRole,
        MinimizedRole,
        MaximizedRole,
        FullscreenRole,
    };

    explicit ToplevelModel(QObject *parent = nullptr);
    ~ToplevelModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return int(m_tops.size()); }
    bool available() const { return m_available; }

    Q_INVOKABLE void activate(int row);
    Q_INVOKABLE void toggleMinimize(int row);
    Q_INVOKABLE void closeWindow(int row);
    Q_INVOKABLE void toggleMaximize(int row);
    Q_INVOKABLE void minimize(int row);

signals:
    void countChanged();
    void availableChanged();

private:
    struct Top;

    static void registryGlobal(void *data, wl_registry *registry, uint32_t name,
                               const char *interface, uint32_t version);
    static void registryRemove(void *data, wl_registry *registry, uint32_t name);

    static void managerToplevel(void *data, zwlr_foreign_toplevel_manager_v1 *manager,
                                zwlr_foreign_toplevel_handle_v1 *handle);
    static void managerFinished(void *data, zwlr_foreign_toplevel_manager_v1 *manager);

    static void topTitle(void *data, zwlr_foreign_toplevel_handle_v1 *handle, const char *title);
    static void topAppId(void *data, zwlr_foreign_toplevel_handle_v1 *handle, const char *appId);
    static void topOutputEnter(void *data, zwlr_foreign_toplevel_handle_v1 *handle,
                               wl_output *output);
    static void topOutputLeave(void *data, zwlr_foreign_toplevel_handle_v1 *handle,
                               wl_output *output);
    static void topState(void *data, zwlr_foreign_toplevel_handle_v1 *handle, wl_array *state);
    static void topDone(void *data, zwlr_foreign_toplevel_handle_v1 *handle);
    static void topClosed(void *data, zwlr_foreign_toplevel_handle_v1 *handle);
    static void topParent(void *data, zwlr_foreign_toplevel_handle_v1 *handle,
                          zwlr_foreign_toplevel_handle_v1 *parent);

    void setupWayland();
    void teardownWayland();
    void dispatchPending();
    int rowFor(const zwlr_foreign_toplevel_handle_v1 *handle) const;
    void emitRowChanged(int row);
    void removeTop(zwlr_foreign_toplevel_handle_v1 *handle);

    wl_display *m_display = nullptr;
    wl_event_queue *m_queue = nullptr;
    wl_registry *m_registry = nullptr;
    wl_seat *m_seat = nullptr;
    zwlr_foreign_toplevel_manager_v1 *m_manager = nullptr;
    QVector<Top *> m_tops;
    QTimer m_pollTimer;
    bool m_available = false;

    // --- Hyprland: estado de minimizado -------------------------------------
    // Hyprland no tiene minimizar nativo. Las ventanas minimizadas viven en el
    // workspace 99. El modelo consulta hyprctl periódicamente para saber qué
    // ventana está realmente oculta, en lugar de fiarse de una lista local que
    // puede quedar desincronizada (p. ej. al minimizar desde hyprbars).
    QTimer m_hyprTimer;
    bool m_hyprFirstPoll = true;
    QSet<QString> m_knownAddresses;
    QString m_lastNormalWorkspace = QStringLiteral("1");
    void pollHyprland();
};
