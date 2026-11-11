#include "systemmanager.h"

#include "launchenvironment.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusVariant>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTimer>

#include <csignal>
#include <unistd.h>

#include <algorithm>

namespace
{

constexpr auto NmService = "org.freedesktop.NetworkManager";
constexpr auto NmPath = "/org/freedesktop/NetworkManager";
constexpr auto NmInterface = "org.freedesktop.NetworkManager";
constexpr auto DeviceInterface = "org.freedesktop.NetworkManager.Device";
constexpr auto WirelessInterface = "org.freedesktop.NetworkManager.Device.Wireless";
constexpr auto AccessPointInterface = "org.freedesktop.NetworkManager.AccessPoint";
constexpr auto PropertiesInterface = "org.freedesktop.DBus.Properties";

constexpr auto LogindService = "org.freedesktop.login1";
constexpr auto LogindPath = "/org/freedesktop/login1";
constexpr auto LogindInterface = "org.freedesktop.login1.Manager";

QDBusMessage propertiesGetAll(const QString &service, const QString &path,
                              const QString &interfaceName)
{
    QDBusMessage message = QDBusMessage::createMethodCall(
        service, path, QString::fromLatin1(PropertiesInterface), QStringLiteral("GetAll"));
    message << interfaceName;
    return message;
}

bool serviceAvailable(QDBusConnection &bus, const char *service)
{
    if (!bus.isConnected()) {
        return false;
    }
    QDBusConnectionInterface *interface = bus.interface();
    if (!interface) {
        return false;
    }
    return interface->isServiceRegistered(QString::fromLatin1(service)).value();
}

// Busca el compositor Wayland en la cadena de procesos padre. Terminarlo de
// forma ordenada cierra la sesión sin depender de la autenticación de polkit.
QString compositorAncestor(qint64 *pidOut)
{
    static const QStringList known = {
        QStringLiteral("Hyprland"),
        QStringLiteral("labwc"),
        QStringLiteral("wayfire"),
        QStringLiteral("sway"),
        QStringLiteral("kwin_wayland"),
        QStringLiteral("mutter"),
    };

    qint64 pid = QCoreApplication::applicationPid();
    for (int depth = 0; depth < 16 && pid > 1; ++depth) {
        QFile comm(QStringLiteral("/proc/%1/comm").arg(pid));
        if (comm.open(QIODevice::ReadOnly)) {
            const QString name = QString::fromLocal8Bit(comm.readAll()).trimmed();
            if (known.contains(name)) {
                if (pidOut) {
                    *pidOut = pid;
                }
                return name;
            }
        }

        QFile status(QStringLiteral("/proc/%1/status").arg(pid));
        if (!status.open(QIODevice::ReadOnly)) {
            break;
        }
        qint64 parent = 0;
        while (!status.atEnd()) {
            const QByteArray line = status.readLine();
            if (line.startsWith("PPid:")) {
                parent = line.mid(5).trimmed().toLongLong();
                break;
            }
        }
        if (parent <= 0 || parent == pid) {
            break;
        }
        pid = parent;
    }
    return {};
}

QString deviceTypeLabel(uint type)
{
    switch (type) {
    case 1:
        return QStringLiteral("Ethernet");
    case 2:
        return QStringLiteral("Wi-Fi");
    case 5:
        return QStringLiteral("Bluetooth");
    case 8:
        return QStringLiteral("Módem");
    case 9:
        return QStringLiteral("InfiniBand");
    case 10:
        return QStringLiteral("Bond");
    case 11:
        return QStringLiteral("VLAN");
    case 12:
        return QStringLiteral("ADSL");
    case 13:
        return QStringLiteral("Puente");
    case 14:
        return QStringLiteral("Genérica");
    case 16:
        return QStringLiteral("TUN");
    case 17:
        return QStringLiteral("IP tunnel");
    case 18:
        return QStringLiteral("MACVLAN");
    case 19:
        return QStringLiteral("VXLAN");
    case 20:
        return QStringLiteral("Veth");
    case 23:
        return QStringLiteral("PPP");
    default:
        return QStringLiteral("Red");
    }
}

QString deviceStateLabel(uint state)
{
    switch (state) {
    case 10:
        return QStringLiteral("No gestionada");
    case 20:
        return QStringLiteral("No disponible");
    case 30:
        return QStringLiteral("Desconectada");
    case 40:
    case 50:
    case 60:
    case 70:
    case 80:
    case 90:
        return QStringLiteral("Conectando");
    case 100:
        return QStringLiteral("Conectada");
    case 110:
        return QStringLiteral("Desconectando");
    case 120:
        return QStringLiteral("Error");
    default:
        return QStringLiteral("Desconocida");
    }
}

QString connectivityLabel(uint connectivity)
{
    switch (connectivity) {
    case 1:
        return QStringLiteral("Sin conexión");
    case 2:
        return QStringLiteral("Portal cautivo");
    case 3:
        return QStringLiteral("Limitada");
    case 4:
        return QStringLiteral("Completa");
    default:
        return QStringLiteral("Desconocida");
    }
}

QString bandLabel(uint frequency)
{
    if (frequency == 0) {
        return {};
    }
    if (frequency < 2500) {
        return QStringLiteral("2.4 GHz");
    }
    if (frequency < 5925) {
        return QStringLiteral("5 GHz");
    }
    return QStringLiteral("6 GHz");
}

} // namespace

struct SystemManager::NetworkScan
{
    bool requestScan = false;
    bool failed = false;
    quint64 generation = 0;

    QList<QDBusObjectPath> devices;
    int deviceIndex = 0;
    QVariantList interfaces;
    QStringList wifiDevices;

    QList<QDBusObjectPath> accessPoints;
    int accessPointListIndex = 0;
    int accessPointIndex = 0;
    QMap<QString, QVariantMap> networks;
};

SystemManager::SystemManager(QObject *parent)
    : QObject(parent)
{
    m_pactl = QStandardPaths::findExecutable(QStringLiteral("pactl"));
    m_wpctl = QStandardPaths::findExecutable(QStringLiteral("wpctl"));
    m_volumeAvailable = !m_pactl.isEmpty() || !m_wpctl.isEmpty();

    m_volumeProcess = new QProcess(this);
    m_volumeProcess->setProcessChannelMode(QProcess::MergedChannels);

    connect(m_volumeProcess, &QProcess::finished, this,
            [this](int exitCode, QProcess::ExitStatus status) {
                if (status != QProcess::NormalExit) {
                    m_volumeStage = VolumeStage::Idle;
                    return;
                }

                const QString output =
                    QString::fromLocal8Bit(m_volumeProcess->readAll()).trimmed();

                switch (m_volumeStage) {
                case VolumeStage::PactlVolume: {
                    static const QRegularExpression percent(
                        QStringLiteral(R"((\d+)%)"));
                    const auto match = percent.match(output);
                    if (exitCode == 0 && match.hasMatch()) {
                        const int value = match.captured(1).toInt();
                        if (value != m_volume) {
                            m_volume = qBound(0, value, 150);
                            emit volumeChanged();
                        }
                    }

                    if (!m_pactl.isEmpty() && exitCode == 0) {
                        m_volumeStage = VolumeStage::PactlMute;
                        m_volumeProcess->start(
                            m_pactl,
                            {QStringLiteral("get-sink-mute"),
                             QStringLiteral("@DEFAULT_SINK@")});
                    } else {
                        m_volumeStage = VolumeStage::Idle;
                    }
                    break;
                }

                case VolumeStage::PactlMute: {
                    const bool muted =
                        exitCode == 0 && output.contains(QLatin1String("yes"),
                                                         Qt::CaseInsensitive);
                    if (muted != m_muted) {
                        m_muted = muted;
                        emit mutedChanged();
                    }
                    m_volumeStage = VolumeStage::Idle;
                    break;
                }

                case VolumeStage::Wpctl: {
                    static const QRegularExpression volume(
                        QStringLiteral(R"(Volume:\s*([0-9]*\.?[0-9]+))"));
                    const auto match = volume.match(output);
                    if (exitCode == 0 && match.hasMatch()) {
                        const int value = qRound(match.captured(1).toDouble() * 100.0);
                        if (value != m_volume) {
                            m_volume = qBound(0, value, 150);
                            emit volumeChanged();
                        }
                    }

                    const bool muted =
                        exitCode == 0 && output.contains(QLatin1String("[MUTED]"),
                                                         Qt::CaseInsensitive);
                    if (muted != m_muted) {
                        m_muted = muted;
                        emit mutedChanged();
                    }
                    m_volumeStage = VolumeStage::Idle;
                    break;
                }

                case VolumeStage::Idle:
                    break;
                }
            });

    connect(m_volumeProcess, &QProcess::errorOccurred, this,
            [this](QProcess::ProcessError error) {
                if (error == QProcess::FailedToStart) {
                    m_volumeStage = VolumeStage::Idle;
                }
            });

    m_volumeTimer = new QTimer(this);
    m_volumeTimer->setInterval(4000);
    connect(m_volumeTimer, &QTimer::timeout, this, &SystemManager::refreshVolume);
    m_volumeTimer->start();

    m_networkTimer = new QTimer(this);
    m_networkTimer->setInterval(15000);
    connect(m_networkTimer, &QTimer::timeout, this, [this] {
        if (!m_scanning) {
            beginNetworkRefresh(false);
        }
    });
    m_networkTimer->start();

    QTimer::singleShot(0, this, &SystemManager::refreshVolume);
    QTimer::singleShot(0, this, &SystemManager::fetchCapabilities);
    QTimer::singleShot(0, this, [this] { beginNetworkRefresh(false); });
}

SystemManager::~SystemManager() = default;

// ---------------------------------------------------------------------------
// systemd-logind
// ---------------------------------------------------------------------------

void SystemManager::powerOff()
{
    callLogind(QStringLiteral("PowerOff"), true, tr("Apagar"));
}

void SystemManager::reboot()
{
    callLogind(QStringLiteral("Reboot"), true, tr("Reiniciar"));
}

void SystemManager::suspend()
{
    callLogind(QStringLiteral("Suspend"), true, tr("Suspender"));
}

void SystemManager::lock()
{
    // El bloqueador propio es una superficie layer-shell Overlay con PAM. Se
    // busca en PATH y, como respaldo, junto al binario que lanzó esta acción.
    QString locker = QStandardPaths::findExecutable(QStringLiteral("stellar-lock"));
    if (locker.isEmpty()) {
        const QString sibling = QCoreApplication::applicationDirPath()
            + QStringLiteral("/stellar-lock");
        if (QFileInfo::exists(sibling)) {
            locker = sibling;
        }
    }

    if (locker.isEmpty()) {
        emit errorOccurred(tr("No se encontró el bloqueador de pantalla (stellar-lock)"));
        return;
    }

    if (!Stellar::startDetached(locker, {}, QDir::homePath())) {
        emit errorOccurred(tr("No se pudo iniciar el bloqueador de pantalla"));
        return;
    }

    emit actionStarted(tr("Bloquear"));
}

void SystemManager::logout()
{
    qint64 compositorPid = 0;
    const QString compositor = compositorAncestor(&compositorPid);

    // Hyprland expone un cierre limpio por IPC; así su lanzador no lo
    // interpreta como un fallo y no lo reinicia.
    if (compositor == QLatin1String("Hyprland")) {
        const QString hyprctl = QStandardPaths::findExecutable(QStringLiteral("hyprctl"));
        if (!hyprctl.isEmpty()) {
            QProcess process;
            process.start(hyprctl, {QStringLiteral("dispatch"), QStringLiteral("exit")});
            if (process.waitForFinished(2000) && process.exitCode() == 0) {
                emit actionStarted(tr("Cerrar sesión"));
                return;
            }
        }
    }

    // KWin ofrece un cierre de sesión ordenado por DBus (con su agente polkit).
    if (compositor == QLatin1String("kwin_wayland")) {
        const QStringList tools = {QStringLiteral("qdbus6"), QStringLiteral("qdbus")};
        for (const QString &tool : tools) {
            const QString qdbus = QStandardPaths::findExecutable(tool);
            if (qdbus.isEmpty()) {
                continue;
            }
            QProcess process;
            process.start(qdbus, {QStringLiteral("org.kde.Shutdown"),
                                  QStringLiteral("/Shutdown"),
                                  QStringLiteral("org.kde.Shutdown.logout")});
            if (process.waitForFinished(2000) && process.exitCode() == 0) {
                emit actionStarted(tr("Cerrar sesión"));
                return;
            }
        }
    }

    // labwc, Wayfire, Sway… terminan la sesión al recibir SIGTERM. El proceso
    // pertenece a esta sesión (es un ancestro del shell), no a otra.
    if (compositorPid > 1) {
        if (::kill(pid_t(compositorPid), SIGTERM) == 0) {
            emit actionStarted(tr("Cerrar sesión"));
            return;
        }
    }

    // Respaldo genérico: logind. Puede pedir autenticación por polkit.
    const QString script = QStringLiteral(
        "if [ -n \"$XDG_SESSION_ID\" ]; then "
        "  loginctl terminate-session \"$XDG_SESSION_ID\" 2>/dev/null || "
        "  loginctl terminate-user \"$USER\"; "
        "else "
        "  loginctl terminate-user \"$USER\"; "
        "fi");

    auto *process = new QProcess(this);
    connect(process, &QProcess::errorOccurred, this,
            [this, process](QProcess::ProcessError error) {
                if (error == QProcess::FailedToStart) {
                    process->deleteLater();
                    emit errorOccurred(tr("No se pudo ejecutar loginctl"));
                }
            });
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this, process](int exitCode, QProcess::ExitStatus status) {
                process->deleteLater();
                if (status != QProcess::NormalExit || exitCode != 0) {
                    emit errorOccurred(tr("No se pudo cerrar la sesión"));
                } else {
                    emit actionStarted(tr("Cerrar sesión"));
                }
            });

    process->start(QStringLiteral("/bin/sh"), {QStringLiteral("-c"), script}, QIODevice::ReadOnly);
}

void SystemManager::callLogind(const QString &method, bool interactive,
                               const QString &label)
{
    QDBusConnection bus = QDBusConnection::systemBus();
    if (!serviceAvailable(bus, LogindService)) {
        emit errorOccurred(tr("systemd-logind no está disponible en el bus del sistema"));
        return;
    }

    QDBusMessage message = QDBusMessage::createMethodCall(
        QString::fromLatin1(LogindService), QString::fromLatin1(LogindPath),
        QString::fromLatin1(LogindInterface), method);
    if (interactive) {
        message << true;
    }

    auto *watcher = new QDBusPendingCallWatcher(bus.asyncCall(message), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this, watcher, label](QDBusPendingCallWatcher *call) {
                const QDBusPendingReply<> reply = *call;
                call->deleteLater();
                if (reply.isError()) {
                    emit errorOccurred(tr("No se pudo completar «%1»: %2")
                                           .arg(label, reply.error().message()));
                } else {
                    emit actionStarted(label);
                }
            });
}

void SystemManager::fetchCapabilities()
{
    QDBusConnection bus = QDBusConnection::systemBus();
    if (!serviceAvailable(bus, LogindService)) {
        const bool hadCapabilities = m_canPowerOff || m_canReboot || m_canSuspend;
        m_canPowerOff = false;
        m_canReboot = false;
        m_canSuspend = false;
        if (hadCapabilities) {
            emit capabilitiesChanged();
        }
        return;
    }

    auto *watcher = new QDBusPendingCallWatcher(
        bus.asyncCall(propertiesGetAll(QString::fromLatin1(LogindService),
                                       QString::fromLatin1(LogindPath),
                                       QString::fromLatin1(LogindInterface))),
        this);

    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this, watcher](QDBusPendingCallWatcher *call) {
                const QDBusPendingReply<QVariantMap> reply = *call;
                call->deleteLater();
                if (reply.isError()) {
                    return;
                }

                const QVariantMap properties = reply.value();
                const auto allowed = [](const QVariant &value) {
                    const QString text = value.toString();
                    return text != QLatin1String("no") && text != QLatin1String("na");
                };

                const bool powerOff = allowed(properties.value(QStringLiteral("CanPowerOff")));
                const bool reboot = allowed(properties.value(QStringLiteral("CanReboot")));
                const bool suspend = allowed(properties.value(QStringLiteral("CanSuspend")));

                if (powerOff == m_canPowerOff && reboot == m_canReboot
                    && suspend == m_canSuspend) {
                    return;
                }

                m_canPowerOff = powerOff;
                m_canReboot = reboot;
                m_canSuspend = suspend;
                emit capabilitiesChanged();
            });
}

// ---------------------------------------------------------------------------
// PulseAudio / PipeWire
// ---------------------------------------------------------------------------

void SystemManager::refreshVolume()
{
    if (!m_volumeProcess || m_volumeProcess->state() != QProcess::NotRunning) {
        return;
    }

    if (!m_pactl.isEmpty()) {
        m_volumeStage = VolumeStage::PactlVolume;
        m_volumeProcess->start(m_pactl,
                               {QStringLiteral("get-sink-volume"),
                                QStringLiteral("@DEFAULT_SINK@")});
    } else if (!m_wpctl.isEmpty()) {
        m_volumeStage = VolumeStage::Wpctl;
        m_volumeProcess->start(m_wpctl,
                               {QStringLiteral("get-volume"),
                                QStringLiteral("@DEFAULT_AUDIO_SINK@")});
    }
}

void SystemManager::applyVolume(int percent)
{
    percent = qBound(0, percent, 150);

    if (m_muted && percent > 0) {
        setMuted(false);
    }

    bool started = false;
    if (!m_pactl.isEmpty()) {
        started = QProcess::startDetached(
            m_pactl,
            {QStringLiteral("set-sink-volume"), QStringLiteral("@DEFAULT_SINK@"),
             QString::number(percent) + QLatin1Char('%')});
    } else if (!m_wpctl.isEmpty()) {
        started = QProcess::startDetached(
            m_wpctl,
            {QStringLiteral("set-volume"), QStringLiteral("@DEFAULT_AUDIO_SINK@"),
             QString::number(percent) + QLatin1Char('%')});
    }

    if (!started) {
        emit errorOccurred(
            tr("No hay un mezclador PulseAudio/PipeWire disponible (pactl/wpctl)"));
        return;
    }

    if (percent != m_volume) {
        m_volume = percent;
        emit volumeChanged();
    }

    QTimer::singleShot(80, this, &SystemManager::refreshVolume);
}

void SystemManager::setVolume(int percent)
{
    applyVolume(percent);
}

void SystemManager::increaseVolume(int step)
{
    applyVolume((m_volume < 0 ? 0 : m_volume) + qMax(1, step));
}

void SystemManager::decreaseVolume(int step)
{
    applyVolume((m_volume < 0 ? 0 : m_volume) - qMax(1, step));
}

void SystemManager::toggleMute()
{
    setMuted(!m_muted);
}

void SystemManager::setMuted(bool muted)
{
    bool started = false;
    if (!m_pactl.isEmpty()) {
        started = QProcess::startDetached(
            m_pactl,
            {QStringLiteral("set-sink-mute"), QStringLiteral("@DEFAULT_SINK@"),
             muted ? QStringLiteral("1") : QStringLiteral("0")});
    } else if (!m_wpctl.isEmpty()) {
        started = QProcess::startDetached(
            m_wpctl,
            {QStringLiteral("set-mute"), QStringLiteral("@DEFAULT_AUDIO_SINK@"),
             muted ? QStringLiteral("1") : QStringLiteral("0")});
    }

    if (!started) {
        emit errorOccurred(
            tr("No hay un mezclador PulseAudio/PipeWire disponible (pactl/wpctl)"));
        return;
    }

    if (muted != m_muted) {
        m_muted = muted;
        emit mutedChanged();
    }

    QTimer::singleShot(80, this, &SystemManager::refreshVolume);
}

// ---------------------------------------------------------------------------
// NetworkManager (DBus)
// ---------------------------------------------------------------------------

void SystemManager::refreshNetworks()
{
    beginNetworkRefresh(false);
}

void SystemManager::scanWifi()
{
    beginNetworkRefresh(true);
}

void SystemManager::connectToWifi(const QString &ssid, const QString &password)
{
    if (ssid.isEmpty()) {
        return;
    }

    QStringList arguments;
    arguments << QStringLiteral("device") << QStringLiteral("wifi")
              << QStringLiteral("connect") << ssid;
    if (!password.isEmpty()) {
        arguments << QStringLiteral("password") << password;
    }

    QProcess *process = new QProcess(this);
    process->setProcessChannelMode(QProcess::MergedChannels);
    connect(process, &QProcess::finished, this, [this, process, ssid](int exitCode,
                                                                     QProcess::ExitStatus status) {
        const QString output = QString::fromLocal8Bit(process->readAll()).trimmed();
        process->deleteLater();

        if (status == QProcess::NormalExit && exitCode == 0) {
            emit errorOccurred(tr("Conectado a %1").arg(ssid));
        } else {
            emit errorOccurred(tr("No se pudo conectar a %1: %2")
                                   .arg(ssid, output.isEmpty() ? tr("error desconocido") : output));
        }

        QTimer::singleShot(1200, this, &SystemManager::refreshNetworks);
    });
    process->start(QStringLiteral("nmcli"), arguments);
}

void SystemManager::setWifiEnabled(bool enabled)
{
    QDBusConnection bus = QDBusConnection::systemBus();
    if (!serviceAvailable(bus, NmService)) {
        emit errorOccurred(tr("NetworkManager no está disponible en el bus del sistema"));
        return;
    }

    QDBusMessage message = QDBusMessage::createMethodCall(
        QString::fromLatin1(NmService), QString::fromLatin1(NmPath),
        QString::fromLatin1(PropertiesInterface), QStringLiteral("Set"));
    message << QString::fromLatin1(NmInterface) << QStringLiteral("WirelessEnabled")
            << QVariant::fromValue(QDBusVariant(enabled));

    auto *watcher = new QDBusPendingCallWatcher(bus.asyncCall(message), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this, watcher, enabled](QDBusPendingCallWatcher *call) {
                const QDBusPendingReply<> reply = *call;
                call->deleteLater();
                if (reply.isError()) {
                    emit errorOccurred(tr("No se pudo cambiar el estado de Wi-Fi: %1")
                                           .arg(reply.error().message()));
                    return;
                }

                if (enabled != m_wifiEnabled) {
                    m_wifiEnabled = enabled;
                    emit wifiEnabledChanged();
                }

                QTimer::singleShot(800, this, &SystemManager::refreshNetworks);
            });
}

void SystemManager::toggleWifi()
{
    setWifiEnabled(!m_wifiEnabled);
}

void SystemManager::beginNetworkRefresh(bool requestScan)
{
    ++m_networkGeneration;

    auto scan = QSharedPointer<NetworkScan>::create();
    scan->requestScan = requestScan;
    scan->generation = m_networkGeneration;
    m_networkScan = scan;

    QDBusConnection bus = QDBusConnection::systemBus();
    const bool available = serviceAvailable(bus, NmService);

    if (available != m_networkManagerAvailable) {
        m_networkManagerAvailable = available;
        emit networkManagerAvailableChanged();
    }

    if (!available) {
        if (!m_interfaces.isEmpty()) {
            m_interfaces.clear();
            emit interfacesChanged();
        }
        if (!m_wifiNetworks.isEmpty()) {
            m_wifiNetworks.clear();
            emit wifiNetworksChanged();
        }
        if (m_scanning) {
            m_scanning = false;
            emit scanningChanged();
        }
        m_networkScan.reset();
        return;
    }

    if (requestScan && !m_scanning) {
        m_scanning = true;
        emit scanningChanged();
    }

    auto *stateWatcher = new QDBusPendingCallWatcher(
        bus.asyncCall(propertiesGetAll(QString::fromLatin1(NmService),
                                       QString::fromLatin1(NmPath),
                                       QString::fromLatin1(NmInterface))),
        this);
    connect(stateWatcher, &QDBusPendingCallWatcher::finished, this,
            [this, scan, stateWatcher](QDBusPendingCallWatcher *call) {
                const QDBusPendingReply<QVariantMap> reply = *call;
                call->deleteLater();
                if (scan != m_networkScan || reply.isError()) {
                    return;
                }

                const QVariantMap properties = reply.value();
                const bool enabled = properties.value(QStringLiteral("WirelessEnabled")).toBool();
                if (enabled != m_wifiEnabled) {
                    m_wifiEnabled = enabled;
                    emit wifiEnabledChanged();
                }
                updateConnectivity(properties.value(QStringLiteral("Connectivity")).toUInt());
            });

    QDBusMessage message = QDBusMessage::createMethodCall(
        QString::fromLatin1(NmService), QString::fromLatin1(NmPath),
        QString::fromLatin1(NmInterface), QStringLiteral("GetDevices"));

    auto *watcher = new QDBusPendingCallWatcher(bus.asyncCall(message), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this, scan, watcher](QDBusPendingCallWatcher *call) {
                const QDBusPendingReply<QList<QDBusObjectPath>> reply = *call;
                call->deleteLater();
                if (scan != m_networkScan) {
                    return;
                }

                if (reply.isError()) {
                    scan->failed = true;
                    finishNetworkScan(scan);
                    return;
                }

                scan->devices = reply.value();
                scan->deviceIndex = 0;
                scan->interfaces.clear();
                scan->wifiDevices.clear();
                fetchNextDevice(scan);
            });
}

void SystemManager::fetchNextDevice(const QSharedPointer<NetworkScan> &scan)
{
    if (scan != m_networkScan) {
        return;
    }

    if (scan->deviceIndex >= scan->devices.size()) {
        publishInterfaces(scan);

        scan->accessPoints.clear();
        scan->accessPointListIndex = 0;
        scan->networks.clear();

        if (scan->requestScan) {
            requestScans(scan);
            QTimer::singleShot(1400, this, [this, scan] {
                fetchNextAccessPointList(scan);
            });
        } else {
            fetchNextAccessPointList(scan);
        }
        return;
    }

    const QString path = scan->devices.at(scan->deviceIndex++).path();
    QDBusConnection bus = QDBusConnection::systemBus();
    auto *watcher = new QDBusPendingCallWatcher(
        bus.asyncCall(propertiesGetAll(QString::fromLatin1(NmService), path,
                                       QString::fromLatin1(DeviceInterface))),
        this);

    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this, scan, watcher, path](QDBusPendingCallWatcher *call) {
                const QDBusPendingReply<QVariantMap> reply = *call;
                call->deleteLater();
                if (scan != m_networkScan) {
                    return;
                }

                if (!reply.isError()) {
                    const QVariantMap properties = reply.value();
                    const uint type = properties.value(QStringLiteral("DeviceType")).toUInt();
                    const uint state = properties.value(QStringLiteral("State")).toUInt();

                    QVariantMap entry;
                    entry.insert(QStringLiteral("name"),
                                 properties.value(QStringLiteral("Interface")).toString());
                    entry.insert(QStringLiteral("path"), path);
                    entry.insert(QStringLiteral("driver"),
                                 properties.value(QStringLiteral("Driver")).toString());
                    entry.insert(QStringLiteral("ip"),
                                 properties.value(QStringLiteral("IpInterface")).toString());
                    entry.insert(QStringLiteral("type"), deviceTypeLabel(type));
                    entry.insert(QStringLiteral("typeId"), type);
                    entry.insert(QStringLiteral("state"), deviceStateLabel(state));
                    entry.insert(QStringLiteral("stateId"), state);
                    entry.insert(QStringLiteral("wireless"), type == 2);
                    entry.insert(QStringLiteral("connected"), state == 100);
                    scan->interfaces.append(entry);

                    if (type == 2) {
                        scan->wifiDevices.append(path);
                    }
                }

                fetchNextDevice(scan);
            });
}

void SystemManager::publishInterfaces(const QSharedPointer<NetworkScan> &scan)
{
    if (scan != m_networkScan) {
        return;
    }

    m_interfaces = scan->interfaces;
    emit interfacesChanged();
}

void SystemManager::requestScans(const QSharedPointer<NetworkScan> &scan)
{
    if (scan != m_networkScan) {
        return;
    }

    QDBusConnection bus = QDBusConnection::systemBus();
    for (const QString &path : scan->wifiDevices) {
        QDBusMessage message = QDBusMessage::createMethodCall(
            QString::fromLatin1(NmService), path,
            QString::fromLatin1(WirelessInterface), QStringLiteral("RequestScan"));
        message << QVariantMap();

        auto *watcher = new QDBusPendingCallWatcher(bus.asyncCall(message), this);
        connect(watcher, &QDBusPendingCallWatcher::finished, this,
                [this, watcher](QDBusPendingCallWatcher *call) {
                    const QDBusPendingReply<> reply = *call;
                    call->deleteLater();
                    if (reply.isError()) {
                        emit errorOccurred(tr("NetworkManager no pudo escanear: %1")
                                               .arg(reply.error().message()));
                    }
                });
    }
}

void SystemManager::fetchNextAccessPointList(const QSharedPointer<NetworkScan> &scan)
{
    if (scan != m_networkScan) {
        return;
    }

    if (scan->accessPointListIndex >= scan->wifiDevices.size()) {
        scan->accessPointIndex = 0;
        fetchNextAccessPoint(scan);
        return;
    }

    const QString path = scan->wifiDevices.at(scan->accessPointListIndex++);
    QDBusConnection bus = QDBusConnection::systemBus();

    QDBusMessage message = QDBusMessage::createMethodCall(
        QString::fromLatin1(NmService), path,
        QString::fromLatin1(WirelessInterface), QStringLiteral("GetAccessPoints"));

    auto *watcher = new QDBusPendingCallWatcher(bus.asyncCall(message), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this, scan, watcher](QDBusPendingCallWatcher *call) {
                const QDBusPendingReply<QList<QDBusObjectPath>> reply = *call;
                call->deleteLater();
                if (scan != m_networkScan) {
                    return;
                }

                if (!reply.isError()) {
                    scan->accessPoints.append(reply.value());
                }
                fetchNextAccessPointList(scan);
            });
}

void SystemManager::fetchNextAccessPoint(const QSharedPointer<NetworkScan> &scan)
{
    if (scan != m_networkScan) {
        return;
    }

    if (scan->accessPointIndex >= scan->accessPoints.size()) {
        finishNetworkScan(scan);
        return;
    }

    const QString path = scan->accessPoints.at(scan->accessPointIndex++).path();
    QDBusConnection bus = QDBusConnection::systemBus();

    auto *watcher = new QDBusPendingCallWatcher(
        bus.asyncCall(propertiesGetAll(QString::fromLatin1(NmService), path,
                                       QString::fromLatin1(AccessPointInterface))),
        this);

    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this, scan, watcher, path](QDBusPendingCallWatcher *call) {
                const QDBusPendingReply<QVariantMap> reply = *call;
                call->deleteLater();
                if (scan != m_networkScan) {
                    return;
                }

                if (!reply.isError()) {
                    const QVariantMap properties = reply.value();
                    const QByteArray ssid =
                        properties.value(QStringLiteral("Ssid")).toByteArray();

                    if (!ssid.isEmpty()) {
                        const QString name = QString::fromUtf8(ssid);
                        const int strength =
                            static_cast<int>(properties.value(QStringLiteral("Strength")).toUInt());
                        const uint frequency =
                            properties.value(QStringLiteral("Frequency")).toUInt();
                        const uint flags =
                            properties.value(QStringLiteral("Flags")).toUInt();
                        const uint wpaFlags =
                            properties.value(QStringLiteral("WpaFlags")).toUInt();
                        const uint rsnFlags =
                            properties.value(QStringLiteral("RsnFlags")).toUInt();

                        QString security = QStringLiteral("Abierta");
                        if (rsnFlags != 0) {
                            security = QStringLiteral("WPA2/WPA3");
                        } else if (wpaFlags != 0) {
                            security = QStringLiteral("WPA");
                        } else if ((flags & 0x1u) != 0) {
                            security = QStringLiteral("WEP");
                        }

                        QVariantMap network;
                        network.insert(QStringLiteral("ssid"), name);
                        network.insert(QStringLiteral("strength"), strength);
                        network.insert(QStringLiteral("frequency"), frequency);
                        network.insert(QStringLiteral("band"), bandLabel(frequency));
                        network.insert(QStringLiteral("secured"),
                                       security != QLatin1String("Abierta"));
                        network.insert(QStringLiteral("security"), security);
                        network.insert(QStringLiteral("path"), path);

                        const auto existing = scan->networks.constFind(name);
                        if (existing == scan->networks.cend()
                            || strength > existing.value()
                                             .value(QStringLiteral("strength"))
                                             .toInt()) {
                            scan->networks.insert(name, network);
                        }
                    }
                }

                fetchNextAccessPoint(scan);
            });
}

void SystemManager::finishNetworkScan(const QSharedPointer<NetworkScan> &scan)
{
    if (scan != m_networkScan) {
        return;
    }

    if (!scan->failed) {
        QVariantList networks;
        networks.reserve(scan->networks.size());
        for (auto it = scan->networks.cbegin(); it != scan->networks.cend(); ++it) {
            networks.append(it.value());
        }

        std::sort(networks.begin(), networks.end(),
                  [](const QVariant &left, const QVariant &right) {
                      return left.toMap().value(QStringLiteral("strength")).toInt()
                          > right.toMap().value(QStringLiteral("strength")).toInt();
                  });

        m_wifiNetworks = networks;
        emit wifiNetworksChanged();
    }

    m_networkScan.reset();

    if (m_scanning) {
        m_scanning = false;
        emit scanningChanged();
    }
}

void SystemManager::updateConnectivity(uint connectivity)
{
    const QString label = connectivityLabel(connectivity);
    if (label != m_connectivity) {
        m_connectivity = label;
        emit connectivityChanged();
    }
}
