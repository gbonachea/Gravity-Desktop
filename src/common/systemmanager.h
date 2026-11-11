#pragma once

#include <QObject>
#include <QSharedPointer>
#include <QStringList>
#include <QVariantList>

class QProcess;
class QTimer;

/*!
 * Native system backend shared by the panel and the start menu.
 *
 *  - Power management through systemd-logind DBus (org.freedesktop.login1).
 *  - Volume control through PulseAudio/PipeWire (pactl, wpctl as fallback).
 *  - Network interface discovery and Wi-Fi scanning through NetworkManager
 *    DBus (org.freedesktop.NetworkManager).
 *
 * Every action is Q_INVOKABLE so the existing QML buttons can call it directly.
 * DBus calls are asynchronous to keep the shell responsive while polkit or
 * NetworkManager reply.
 */
class SystemManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool canPowerOff READ canPowerOff NOTIFY capabilitiesChanged)
    Q_PROPERTY(bool canReboot READ canReboot NOTIFY capabilitiesChanged)
    Q_PROPERTY(bool canSuspend READ canSuspend NOTIFY capabilitiesChanged)

    Q_PROPERTY(bool volumeAvailable READ volumeAvailable NOTIFY volumeAvailableChanged)
    Q_PROPERTY(int volume READ volume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ muted NOTIFY mutedChanged)

    Q_PROPERTY(bool networkManagerAvailable READ networkManagerAvailable
                   NOTIFY networkManagerAvailableChanged)
    Q_PROPERTY(bool wifiEnabled READ wifiEnabled NOTIFY wifiEnabledChanged)
    Q_PROPERTY(QString connectivity READ connectivity NOTIFY connectivityChanged)
    Q_PROPERTY(QVariantList interfaces READ interfaces NOTIFY interfacesChanged)
    Q_PROPERTY(QVariantList wifiNetworks READ wifiNetworks NOTIFY wifiNetworksChanged)
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)

public:
    explicit SystemManager(QObject *parent = nullptr);
    ~SystemManager() override;

    bool canPowerOff() const { return m_canPowerOff; }
    bool canReboot() const { return m_canReboot; }
    bool canSuspend() const { return m_canSuspend; }

    bool volumeAvailable() const { return m_volumeAvailable; }
    int volume() const { return m_volume; }
    bool muted() const { return m_muted; }

    bool networkManagerAvailable() const { return m_networkManagerAvailable; }
    bool wifiEnabled() const { return m_wifiEnabled; }
    QString connectivity() const { return m_connectivity; }
    QVariantList interfaces() const { return m_interfaces; }
    QVariantList wifiNetworks() const { return m_wifiNetworks; }
    bool scanning() const { return m_scanning; }

    // --- systemd-logind -----------------------------------------------------
    Q_INVOKABLE void powerOff();
    Q_INVOKABLE void reboot();
    Q_INVOKABLE void suspend();
    Q_INVOKABLE void lock();
    Q_INVOKABLE void logout();

    // --- PulseAudio / PipeWire ---------------------------------------------
    Q_INVOKABLE void setVolume(int percent);
    Q_INVOKABLE void increaseVolume(int step = 5);
    Q_INVOKABLE void decreaseVolume(int step = 5);
    Q_INVOKABLE void toggleMute();
    Q_INVOKABLE void refreshVolume();

    // --- NetworkManager -----------------------------------------------------
    Q_INVOKABLE void refreshNetworks();
    Q_INVOKABLE void scanWifi();
    Q_INVOKABLE void connectToWifi(const QString &ssid, const QString &password);
    Q_INVOKABLE void setWifiEnabled(bool enabled);
    Q_INVOKABLE void toggleWifi();

signals:
    void capabilitiesChanged();

    void volumeAvailableChanged();
    void volumeChanged();
    void mutedChanged();

    void networkManagerAvailableChanged();
    void wifiEnabledChanged();
    void connectivityChanged();
    void interfacesChanged();
    void wifiNetworksChanged();
    void scanningChanged();

    void actionStarted(const QString &action);
    void errorOccurred(const QString &message);

private:
    struct NetworkScan;

    void callLogind(const QString &method, bool interactive, const QString &label);
    void fetchCapabilities();

    void setMuted(bool muted);
    void applyVolume(int percent);

    void beginNetworkRefresh(bool requestScan);
    void fetchNextDevice(const QSharedPointer<NetworkScan> &scan);
    void fetchNextAccessPointList(const QSharedPointer<NetworkScan> &scan);
    void fetchNextAccessPoint(const QSharedPointer<NetworkScan> &scan);
    void finishNetworkScan(const QSharedPointer<NetworkScan> &scan);
    void requestScans(const QSharedPointer<NetworkScan> &scan);
    void publishInterfaces(const QSharedPointer<NetworkScan> &scan);
    void updateConnectivity(uint connectivity);

    bool m_canPowerOff = false;
    bool m_canReboot = false;
    bool m_canSuspend = false;

    bool m_volumeAvailable = false;
    int m_volume = -1;
    bool m_muted = false;
    QString m_pactl;
    QString m_wpctl;
    QProcess *m_volumeProcess = nullptr;
    QTimer *m_volumeTimer = nullptr;

    enum class VolumeStage {
        Idle,
        PactlVolume,
        PactlMute,
        Wpctl,
    };
    VolumeStage m_volumeStage = VolumeStage::Idle;

    bool m_networkManagerAvailable = false;
    bool m_wifiEnabled = false;
    bool m_scanning = false;
    QString m_connectivity = QStringLiteral("Desconocida");
    QVariantList m_interfaces;
    QVariantList m_wifiNetworks;
    QSharedPointer<NetworkScan> m_networkScan;
    quint64 m_networkGeneration = 0;
    QTimer *m_networkTimer = nullptr;
};
