#include "toplevelmodel.h"

#include "desktopentrymodel.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QStandardPaths>
#include <QTextStream>
#include <QTimer>

#include <algorithm>
#include <cstring>

#if STELLAR_HAVE_FOREIGN_TOPLEVEL

#include <QtGui/qguiapplication_platform.h>

#include <wayland-client.h>

extern "C" {
#include "wlr-foreign-toplevel-management-unstable-v1-client-protocol.h"
}

#endif

struct ToplevelModel::Top
{
    zwlr_foreign_toplevel_handle_v1 *handle = nullptr;
    QString title;
    QString appId;
    QString icon;
    bool active = false;
    bool minimized = false;
    bool maximized = false;
    bool fullscreen = false;
    // Marcado localmente como minimizado (workspace 99). El compositor puede
    // seguir reportándolo como activo; el panel debe mostrarlo como inactivo.
    bool hiddenLocally = false;
    // Dirección de Hyprland (0x…) y workspace real de esta ventana. Se
    // refrescan con hyprctl para que el estado del panel coincida siempre con
    // el compositor, sin importar desde dónde se minimizó.
    QString hyprAddress;
    QString hyprWorkspace;
};

namespace
{

bool onHyprland()
{
    return !qEnvironmentVariableIsEmpty("HYPRLAND_INSTANCE_SIGNATURE");
}

QString hyprctlPath()
{
    static const QString path =
        QStandardPaths::findExecutable(QStringLiteral("hyprctl"));
    return path;
}

QString runHyprctl(const QStringList &args)
{
    const QString tool = hyprctlPath();
    if (tool.isEmpty()) {
        return {};
    }
    QProcess process;
    process.start(tool, args);
    if (!process.waitForFinished(3000)) {
        return {};
    }
    return QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
}

struct HyprWindowInfo
{
    QString address;
    QString workspace;
};

HyprWindowInfo findHyprWindow(const QString &title, const QString &appId)
{
    HyprWindowInfo info;
    const QString json = runHyprctl({QStringLiteral("clients"), QStringLiteral("-j")});
    if (json.isEmpty()) {
        return info;
    }
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !document.isArray()) {
        return info;
    }
    HyprWindowInfo fallback;
    for (const QJsonValue &value : document.array()) {
        const QJsonObject window = value.toObject();
        const QString windowTitle = window.value(QStringLiteral("title")).toString();
        if (windowTitle != title) {
            continue;
        }
        const QString windowClass = window.value(QStringLiteral("class")).toString();
        HyprWindowInfo candidate;
        candidate.address = window.value(QStringLiteral("address")).toString();
        candidate.workspace = window.value(QStringLiteral("workspace"))
                                  .toObject()
                                  .value(QStringLiteral("name"))
                                  .toString();
        if (appId.isEmpty() || windowClass == appId) {
            return candidate;
        }
        if (fallback.address.isEmpty()) {
            fallback = candidate;
        }
    }
    return fallback;
}

QString hyprActiveWorkspace()
{
    const QString json =
        runHyprctl({QStringLiteral("activeworkspace"), QStringLiteral("-j")});
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return QStringLiteral("1");
    }
    const QJsonObject workspace = document.object();
    // El nombre cubre tanto workspaces numéricos ("1", "99") como especiales
    // ("special:minimized") y con nombre. hyprctl acepta ambos al despachar.
    const QString name = workspace.value(QStringLiteral("name")).toString();
    if (!name.isEmpty()) {
        return name;
    }
    return QString::number(workspace.value(QStringLiteral("id")).toInt(1));
}

// Hyprland no tiene minimizar nativo: las ventanas ocultas viven en el
// workspace 99 (o en el antiguo special:minimized).
bool isHiddenWorkspace(const QString &workspace)
{
    return workspace == QLatin1String("99")
        || workspace == QLatin1String("special:minimized");
}

void hyprDispatch(const QStringList &args)
{
    runHyprctl(QStringList{QStringLiteral("dispatch")} + args);
}

QString stellarStateDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericStateLocation)
        + QStringLiteral("/stellar");
}

void logMinimize(const QString &message)
{
    const QString dir = stellarStateDir();
    QDir().mkpath(dir);
    QFile file(dir + QStringLiteral("/minimize.log"));
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << QDateTime::currentDateTime().toString(Qt::ISODate) << " "
            << message << "\n";
    }
}

// Tras minimizar, enfoca una ventana normal (si existe) para que el foco no
// quede en la ventana oculta. Si no hay ninguna, permanece en el workspace
// actual: no se fuerza ningún cambio de escritorio.
void hyprFocusNormalWindow(const QString &fallbackWorkspace)
{
    const QString json = runHyprctl({QStringLiteral("clients"), QStringLiteral("-j")});
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8(), &error);
    if (error.error == QJsonParseError::NoError && document.isArray()) {
        for (const QJsonValue &value : document.array()) {
            const QJsonObject window = value.toObject();
            const QString workspace = window.value(QStringLiteral("workspace"))
                                          .toObject()
                                          .value(QStringLiteral("name"))
                                          .toString();
            const bool mapped = window.value(QStringLiteral("mapped")).toBool();
            const QString address = window.value(QStringLiteral("address")).toString();
            if (mapped && !isHiddenWorkspace(workspace) && !address.isEmpty()) {
                hyprDispatch({QStringLiteral("focuswindow"),
                              QStringLiteral("address:") + address});
                return;
            }
        }
    }
    if (!fallbackWorkspace.isEmpty() && fallbackWorkspace != QLatin1String("99")) {
        hyprDispatch({QStringLiteral("workspace"), fallbackWorkspace});
    }
}

} // namespace

ToplevelModel::ToplevelModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_pollTimer.setInterval(40);
    connect(&m_pollTimer, &QTimer::timeout, this, &ToplevelModel::dispatchPending);
    m_pollTimer.start();

    if (onHyprland()) {
        m_hyprTimer.setInterval(300);
        connect(&m_hyprTimer, &QTimer::timeout, this, &ToplevelModel::pollHyprland);
        m_hyprTimer.start();
        QTimer::singleShot(150, this, &ToplevelModel::pollHyprland);
    }

    setupWayland();
}

ToplevelModel::~ToplevelModel()
{
    teardownWayland();
    qDeleteAll(m_tops);
    m_tops.clear();
}

int ToplevelModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return int(m_tops.size());
}

QVariant ToplevelModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_tops.size()) {
        return {};
    }

    const Top *top = m_tops.at(index.row());
    switch (role) {
    case TitleRole:
        return top->title;
    case AppIdRole:
        return top->appId;
    case IconRole:
        return top->icon.isEmpty() ? QStringLiteral("application-x-executable") : top->icon;
    case ActiveRole:
        return top->active && !top->hiddenLocally;
    case MinimizedRole:
        return top->minimized || top->hiddenLocally;
    case MaximizedRole:
        return top->maximized;
    case FullscreenRole:
        return top->fullscreen;
    default:
        return {};
    }
}

QHash<int, QByteArray> ToplevelModel::roleNames() const
{
    return {
        {TitleRole, "title"},
        {AppIdRole, "appId"},
        {IconRole, "icon"},
        {ActiveRole, "active"},
        {MinimizedRole, "minimized"},
        {MaximizedRole, "maximized"},
        {FullscreenRole, "fullscreen"},
    };
}

void ToplevelModel::setupWayland()
{
#if STELLAR_HAVE_FOREIGN_TOPLEVEL
    if (!QGuiApplication::platformName().startsWith(QLatin1String("wayland"))) {
        return;
    }

    auto *wayland = qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>();
    if (!wayland) {
        return;
    }

    m_display = wayland->display();
    m_seat = wayland->seat();
    if (!m_display) {
        return;
    }

    m_queue = wl_display_create_queue(m_display);
    if (!m_queue) {
        return;
    }

    m_registry = wl_display_get_registry(m_display);
    if (!m_registry) {
        return;
    }
    wl_proxy_set_queue(reinterpret_cast<wl_proxy *>(m_registry), m_queue);

    static const wl_registry_listener registryListener = {
        &ToplevelModel::registryGlobal,
        &ToplevelModel::registryRemove,
    };
    wl_registry_add_listener(m_registry, &registryListener, this);
#endif
}

void ToplevelModel::teardownWayland()
{
#if STELLAR_HAVE_FOREIGN_TOPLEVEL
    for (Top *top : std::as_const(m_tops)) {
        if (top->handle) {
            zwlr_foreign_toplevel_handle_v1_destroy(top->handle);
            top->handle = nullptr;
        }
    }

    if (m_manager) {
        zwlr_foreign_toplevel_manager_v1_destroy(m_manager);
        m_manager = nullptr;
    }

    if (m_registry) {
        wl_registry_destroy(m_registry);
        m_registry = nullptr;
    }

    if (m_queue) {
        wl_event_queue_destroy(m_queue);
        m_queue = nullptr;
    }

    m_display = nullptr;
#endif
}

void ToplevelModel::dispatchPending()
{
#if STELLAR_HAVE_FOREIGN_TOPLEVEL
    if (!m_display) {
        return;
    }

    // Wake the compositor if a request is waiting in the outgoing buffer and
    // then process events that Qt already read from the socket into our queue.
    wl_display_flush(m_display);
    if (m_queue) {
        wl_display_dispatch_queue_pending(m_display, m_queue);
    }
#endif
}

int ToplevelModel::rowFor(const zwlr_foreign_toplevel_handle_v1 *handle) const
{
    for (int i = 0; i < m_tops.size(); ++i) {
        if (m_tops.at(i)->handle == handle) {
            return i;
        }
    }
    return -1;
}

void ToplevelModel::emitRowChanged(int row)
{
    if (row < 0 || row >= m_tops.size()) {
        return;
    }
    const QModelIndex modelIndex = index(row, 0);
    emit dataChanged(modelIndex, modelIndex);
}

void ToplevelModel::removeTop(zwlr_foreign_toplevel_handle_v1 *handle)
{
    const int row = rowFor(handle);
    if (row < 0) {
        return;
    }

    beginRemoveRows(QModelIndex(), row, row);
    Top *top = m_tops.takeAt(row);
    if (top->handle) {
        zwlr_foreign_toplevel_handle_v1_destroy(top->handle);
    }
    delete top;
    endRemoveRows();

    emit countChanged();
}

void ToplevelModel::activate(int row)
{
#if STELLAR_HAVE_FOREIGN_TOPLEVEL
    if (row < 0 || row >= m_tops.size() || !m_tops.at(row)->handle) {
        return;
    }
    if (m_seat) {
        zwlr_foreign_toplevel_handle_v1_activate(m_tops.at(row)->handle, m_seat);
    }
#else
    Q_UNUSED(row)
#endif
}

void ToplevelModel::toggleMinimize(int row)
{
#if STELLAR_HAVE_FOREIGN_TOPLEVEL
    if (row < 0 || row >= m_tops.size() || !m_tops.at(row)->handle) {
        return;
    }

    Top *top = m_tops.at(row);

    // Hyprland no tiene minimizar nativo y su foreign-toplevel no refleja el
    // workspace oculto. Usamos la dirección y el workspace que mantiene el
    // sondeo continuo (pollHyprland), con hyprctl como respaldo.
    if (onHyprland()) {
        HyprWindowInfo info;
        if (top->hyprAddress.isEmpty()) {
            info = findHyprWindow(top->title, top->appId);
        }
        const QString address =
            top->hyprAddress.isEmpty() ? info.address : top->hyprAddress;
        const QString workspace =
            top->hyprWorkspace.isEmpty() ? info.workspace : top->hyprWorkspace;

        logMinimize(QStringLiteral("row=%1 title='%2' appId='%3' active=%4 "
                                   "minimized=%5 ws='%6' addr='%7'")
                        .arg(row)
                        .arg(top->title, top->appId)
                        .arg(top->active)
                        .arg(top->minimized)
                        .arg(workspace, address));

        if (!address.isEmpty()) {
            const bool hidden = top->hiddenLocally || top->minimized
                || isHiddenWorkspace(workspace);
            if (hidden) {
                // Restaurar en el workspace activo (nunca en el 99).
                QString target = hyprActiveWorkspace();
                if (isHiddenWorkspace(target) || target.isEmpty()) {
                    target = m_lastNormalWorkspace;
                }
                logMinimize(QStringLiteral("action=restore to ws=%1").arg(target));
                top->hiddenLocally = false;
                top->minimized = false;
                top->hyprWorkspace = target;
                emitRowChanged(row);
                hyprDispatch({QStringLiteral("movetoworkspacesilent"),
                              target + QStringLiteral(",address:") + address});
                hyprDispatch({QStringLiteral("focuswindow"),
                              QStringLiteral("address:") + address});
            } else if (top->active) {
                // Minimizar al workspace oculto 99 y mantener el escritorio
                // actual como activo.
                const QString previous = hyprActiveWorkspace();
                if (!isHiddenWorkspace(previous) && !previous.isEmpty()) {
                    m_lastNormalWorkspace = previous;
                }
                top->hiddenLocally = true;
                top->active = false;
                top->hyprWorkspace = QStringLiteral("99");
                emitRowChanged(row);
                logMinimize(QStringLiteral("action=minimize previousWs=%1").arg(previous));
                hyprDispatch({QStringLiteral("movetoworkspacesilent"),
                              QStringLiteral("99,address:") + address});

                // Si el compositor cambió el workspace activo a 99, volver.
                const QString now = hyprActiveWorkspace();
                if (isHiddenWorkspace(now)) {
                    logMinimize(QStringLiteral("active became %1; switching back to %2")
                                    .arg(now, m_lastNormalWorkspace));
                    hyprDispatch({QStringLiteral("workspace"), m_lastNormalWorkspace});
                } else {
                    hyprFocusNormalWindow(m_lastNormalWorkspace);
                }
            } else {
                logMinimize(QStringLiteral("action=focus"));
                top->hiddenLocally = false;
                emitRowChanged(row);
                hyprDispatch({QStringLiteral("focuswindow"),
                              QStringLiteral("address:") + address});
            }
            return;
        }
        logMinimize(QStringLiteral("action=fallback (no address)"));
    }

    if (top->minimized || top->hiddenLocally) {
        zwlr_foreign_toplevel_handle_v1_unset_minimized(top->handle);
        top->hiddenLocally = false;
        emitRowChanged(row);
        if (m_seat) {
            zwlr_foreign_toplevel_handle_v1_activate(top->handle, m_seat);
        }
    } else if (top->active) {
        zwlr_foreign_toplevel_handle_v1_set_minimized(top->handle);
        top->hiddenLocally = true;
        top->active = false;
        emitRowChanged(row);
    } else {
        top->hiddenLocally = false;
        emitRowChanged(row);
        if (m_seat) {
            zwlr_foreign_toplevel_handle_v1_activate(top->handle, m_seat);
        }
    }
#else
    Q_UNUSED(row)
#endif
}

void ToplevelModel::pollHyprland()
{
#if STELLAR_HAVE_FOREIGN_TOPLEVEL
    if (!onHyprland()) {
        return;
    }

    const QString json = runHyprctl({QStringLiteral("clients"), QStringLiteral("-j")});
    if (json.isEmpty()) {
        return;
    }
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !document.isArray()) {
        return;
    }

    struct ClientInfo {
        QString address;
        QString title;
        QString appId;
        QString workspace;
    };

    QVector<ClientInfo> clients;
    QSet<QString> currentAddresses;
    for (const QJsonValue &value : document.array()) {
        const QJsonObject window = value.toObject();
        ClientInfo info;
        info.address = window.value(QStringLiteral("address")).toString();
        if (info.address.isEmpty()) {
            continue;
        }
        info.title = window.value(QStringLiteral("title")).toString();
        info.appId = window.value(QStringLiteral("class")).toString();
        info.workspace = window.value(QStringLiteral("workspace"))
                             .toObject()
                             .value(QStringLiteral("name"))
                             .toString();
        clients.append(info);
        currentAddresses.insert(info.address);
    }

    const QString activeWorkspace = hyprActiveWorkspace();
    if (!isHiddenWorkspace(activeWorkspace) && !activeWorkspace.isEmpty()) {
        m_lastNormalWorkspace = activeWorkspace;
    }

    // Solo las ventanas que aparecen por primera vez en el workspace oculto se
    // devuelven al escritorio normal. Las que ya estaban allí (minimizadas) no
    // se tocan nunca: ese era el fallo que hacía reaparecer una app minimizada
    // al abrir otra.
    if (!m_hyprFirstPoll) {
        for (const ClientInfo &client : clients) {
            if (isHiddenWorkspace(client.workspace)
                && !m_knownAddresses.contains(client.address)) {
                logMinimize(QStringLiteral("poll: new window '%1' on %2; moving to %3")
                                .arg(client.title, client.workspace,
                                     m_lastNormalWorkspace));
                hyprDispatch({QStringLiteral("movetoworkspacesilent"),
                              m_lastNormalWorkspace + QStringLiteral(",address:")
                                  + client.address});
            }
        }
    }
    m_hyprFirstPoll = false;
    m_knownAddresses = currentAddresses;

    // El workspace oculto nunca debe quedar activo: revelaría las minimizadas.
    if (isHiddenWorkspace(activeWorkspace)) {
        logMinimize(QStringLiteral("poll: active=%1; switching to %2")
                        .arg(activeWorkspace, m_lastNormalWorkspace));
        hyprDispatch({QStringLiteral("workspace"), m_lastNormalWorkspace});
    }

    QHash<QString, QString> workspaceByAddress;
    for (const ClientInfo &client : clients) {
        workspaceByAddress.insert(client.address, client.workspace);
    }

    // Empareja cada toplevel del protocolo con su ventana real de Hyprland.
    QSet<QString> used;
    for (Top *top : std::as_const(m_tops)) {
        if (!top->hyprAddress.isEmpty()
            && currentAddresses.contains(top->hyprAddress)) {
            used.insert(top->hyprAddress);
        } else {
            top->hyprAddress.clear();
        }
    }

    for (int row = 0; row < m_tops.size(); ++row) {
        Top *top = m_tops.at(row);
        if (top->hyprAddress.isEmpty()) {
            int bestIndex = -1;
            int bestScore = 0;
            for (int i = 0; i < clients.size(); ++i) {
                const ClientInfo &client = clients.at(i);
                if (used.contains(client.address)) {
                    continue;
                }
                int score = 0;
                if (!top->appId.isEmpty()
                    && client.appId.compare(top->appId, Qt::CaseInsensitive) == 0) {
                    score += 2;
                }
                if (!top->title.isEmpty() && client.title == top->title) {
                    score += 3;
                }
                if (score > bestScore) {
                    bestScore = score;
                    bestIndex = i;
                }
            }
            if (bestIndex >= 0) {
                top->hyprAddress = clients.at(bestIndex).address;
                used.insert(top->hyprAddress);
            }
        }
        if (top->hyprAddress.isEmpty()) {
            continue;
        }

        const QString workspace = workspaceByAddress.value(top->hyprAddress);
        if (workspace.isEmpty()) {
            continue;
        }
        top->hyprWorkspace = workspace;

        const bool hidden = isHiddenWorkspace(workspace);
        if (hidden != top->hiddenLocally) {
            top->hiddenLocally = hidden;
            if (hidden) {
                top->active = false;
            }
            logMinimize(QStringLiteral("poll: row=%1 '%2' ws=%3 hidden=%4")
                            .arg(row)
                            .arg(top->title, workspace)
                            .arg(hidden));
            emitRowChanged(row);
        }
    }
#endif
}

void ToplevelModel::closeWindow(int row)
{
#if STELLAR_HAVE_FOREIGN_TOPLEVEL
    if (row < 0 || row >= m_tops.size() || !m_tops.at(row)->handle) {
        return;
    }
    zwlr_foreign_toplevel_handle_v1_close(m_tops.at(row)->handle);
#else
    Q_UNUSED(row)
#endif
}

void ToplevelModel::toggleMaximize(int row)
{
#if STELLAR_HAVE_FOREIGN_TOPLEVEL
    if (row < 0 || row >= m_tops.size() || !m_tops.at(row)->handle) {
        return;
    }
    Top *top = m_tops.at(row);
    if (top->maximized) {
        zwlr_foreign_toplevel_handle_v1_unset_maximized(top->handle);
    } else {
        zwlr_foreign_toplevel_handle_v1_set_maximized(top->handle);
    }
#else
    Q_UNUSED(row)
#endif
}

void ToplevelModel::minimize(int row)
{
#if STELLAR_HAVE_FOREIGN_TOPLEVEL
    if (row < 0 || row >= m_tops.size() || !m_tops.at(row)->handle) {
        return;
    }
    zwlr_foreign_toplevel_handle_v1_set_minimized(m_tops.at(row)->handle);
#else
    Q_UNUSED(row)
#endif
}

// ---------------------------------------------------------------------------
// Wayland callbacks
// ---------------------------------------------------------------------------

void ToplevelModel::registryGlobal(void *data, wl_registry *registry, uint32_t name,
                                   const char *interface, uint32_t version)
{
#if STELLAR_HAVE_FOREIGN_TOPLEVEL
    auto *self = static_cast<ToplevelModel *>(data);
    if (std::strcmp(interface, zwlr_foreign_toplevel_manager_v1_interface.name) != 0) {
        return;
    }

    if (self->m_manager) {
        return;
    }

    const uint32_t bindVersion = std::min<uint32_t>(version, 3u);
    self->m_manager = static_cast<zwlr_foreign_toplevel_manager_v1 *>(wl_registry_bind(
        registry, name, &zwlr_foreign_toplevel_manager_v1_interface, bindVersion));
    if (!self->m_manager) {
        return;
    }

    wl_proxy_set_queue(reinterpret_cast<wl_proxy *>(self->m_manager), self->m_queue);

    static const zwlr_foreign_toplevel_manager_v1_listener listener = {
        &ToplevelModel::managerToplevel,
        &ToplevelModel::managerFinished,
    };
    zwlr_foreign_toplevel_manager_v1_add_listener(self->m_manager, &listener, self);

    self->m_available = true;
    emit self->availableChanged();
#else
    Q_UNUSED(data)
    Q_UNUSED(registry)
    Q_UNUSED(name)
    Q_UNUSED(interface)
    Q_UNUSED(version)
#endif
}

void ToplevelModel::registryRemove(void *data, wl_registry *registry, uint32_t name)
{
    Q_UNUSED(data)
    Q_UNUSED(registry)
    Q_UNUSED(name)
}

void ToplevelModel::managerToplevel(void *data, zwlr_foreign_toplevel_manager_v1 *manager,
                                    zwlr_foreign_toplevel_handle_v1 *handle)
{
#if STELLAR_HAVE_FOREIGN_TOPLEVEL
    Q_UNUSED(manager)

    auto *self = static_cast<ToplevelModel *>(data);
    if (!handle) {
        return;
    }

    wl_proxy_set_queue(reinterpret_cast<wl_proxy *>(handle), self->m_queue);

    static const zwlr_foreign_toplevel_handle_v1_listener listener = {
        &ToplevelModel::topTitle,
        &ToplevelModel::topAppId,
        &ToplevelModel::topOutputEnter,
        &ToplevelModel::topOutputLeave,
        &ToplevelModel::topState,
        &ToplevelModel::topDone,
        &ToplevelModel::topClosed,
        &ToplevelModel::topParent,
    };
    zwlr_foreign_toplevel_handle_v1_add_listener(handle, &listener, self);

    auto *top = new Top;
    top->handle = handle;

    const int row = int(self->m_tops.size());
    self->beginInsertRows(QModelIndex(), row, row);
    self->m_tops.append(top);
    self->endInsertRows();
    emit self->countChanged();

    // Sincroniza cuanto antes el estado real de la ventana (workspace oculto,
    // dirección de Hyprland, etc.) con el nuevo toplevel.
    if (onHyprland()) {
        QTimer::singleShot(120, self, &ToplevelModel::pollHyprland);
    }
#else
    Q_UNUSED(data)
    Q_UNUSED(manager)
    Q_UNUSED(handle)
#endif
}

void ToplevelModel::managerFinished(void *data, zwlr_foreign_toplevel_manager_v1 *manager)
{
#if STELLAR_HAVE_FOREIGN_TOPLEVEL
    Q_UNUSED(manager)

    auto *self = static_cast<ToplevelModel *>(data);

    self->beginResetModel();
    while (!self->m_tops.isEmpty()) {
        Top *top = self->m_tops.takeLast();
        if (top->handle) {
            zwlr_foreign_toplevel_handle_v1_destroy(top->handle);
        }
        delete top;
    }
    self->endResetModel();
    emit self->countChanged();

    if (self->m_manager) {
        zwlr_foreign_toplevel_manager_v1_destroy(self->m_manager);
        self->m_manager = nullptr;
    }

    if (self->m_available) {
        self->m_available = false;
        emit self->availableChanged();
    }
#else
    Q_UNUSED(data)
    Q_UNUSED(manager)
#endif
}

void ToplevelModel::topTitle(void *data, zwlr_foreign_toplevel_handle_v1 *handle,
                             const char *title)
{
#if STELLAR_HAVE_FOREIGN_TOPLEVEL
    auto *self = static_cast<ToplevelModel *>(data);
    const int row = self->rowFor(handle);
    if (row < 0) {
        return;
    }
    self->m_tops.at(row)->title = QString::fromUtf8(title ? title : "");
#else
    Q_UNUSED(data)
    Q_UNUSED(handle)
    Q_UNUSED(title)
#endif
}

void ToplevelModel::topAppId(void *data, zwlr_foreign_toplevel_handle_v1 *handle,
                             const char *appId)
{
#if STELLAR_HAVE_FOREIGN_TOPLEVEL
    auto *self = static_cast<ToplevelModel *>(data);
    const int row = self->rowFor(handle);
    if (row < 0) {
        return;
    }
    self->m_tops.at(row)->appId = QString::fromUtf8(appId ? appId : "");
#else
    Q_UNUSED(data)
    Q_UNUSED(handle)
    Q_UNUSED(appId)
#endif
}

void ToplevelModel::topOutputEnter(void *data, zwlr_foreign_toplevel_handle_v1 *handle,
                                   wl_output *output)
{
    Q_UNUSED(data)
    Q_UNUSED(handle)
    Q_UNUSED(output)
}

void ToplevelModel::topOutputLeave(void *data, zwlr_foreign_toplevel_handle_v1 *handle,
                                   wl_output *output)
{
    Q_UNUSED(data)
    Q_UNUSED(handle)
    Q_UNUSED(output)
}

void ToplevelModel::topState(void *data, zwlr_foreign_toplevel_handle_v1 *handle,
                             wl_array *state)
{
#if STELLAR_HAVE_FOREIGN_TOPLEVEL
    auto *self = static_cast<ToplevelModel *>(data);
    const int row = self->rowFor(handle);
    if (row < 0) {
        return;
    }

    Top *top = self->m_tops.at(row);
    top->active = false;
    top->minimized = false;
    top->maximized = false;
    top->fullscreen = false;

    const auto *states = static_cast<const uint32_t *>(state->data);
    const int count = int(state->size / sizeof(uint32_t));
    for (int i = 0; i < count; ++i) {
        switch (states[i]) {
        case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED:
            top->active = true;
            break;
        case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MINIMIZED:
            top->minimized = true;
            break;
        case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MAXIMIZED:
            top->maximized = true;
            break;
        case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_FULLSCREEN:
            top->fullscreen = true;
            break;
        default:
            break;
        }
    }
#else
    Q_UNUSED(data)
    Q_UNUSED(handle)
    Q_UNUSED(state)
#endif
}

void ToplevelModel::topDone(void *data, zwlr_foreign_toplevel_handle_v1 *handle)
{
#if STELLAR_HAVE_FOREIGN_TOPLEVEL
    auto *self = static_cast<ToplevelModel *>(data);
    const int row = self->rowFor(handle);
    if (row < 0) {
        return;
    }

    Top *top = self->m_tops.at(row);
    top->icon = DesktopEntryIndex::instance().iconForAppId(top->appId);
    self->emitRowChanged(row);
#else
    Q_UNUSED(data)
    Q_UNUSED(handle)
#endif
}

void ToplevelModel::topClosed(void *data, zwlr_foreign_toplevel_handle_v1 *handle)
{
#if STELLAR_HAVE_FOREIGN_TOPLEVEL
    auto *self = static_cast<ToplevelModel *>(data);
    self->removeTop(handle);
#else
    Q_UNUSED(data)
    Q_UNUSED(handle)
#endif
}

void ToplevelModel::topParent(void *data, zwlr_foreign_toplevel_handle_v1 *handle,
                              zwlr_foreign_toplevel_handle_v1 *parent)
{
    Q_UNUSED(data)
    Q_UNUSED(handle)
    Q_UNUSED(parent)
}
