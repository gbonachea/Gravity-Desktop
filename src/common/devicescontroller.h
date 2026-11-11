#pragma once

#include <QObject>
#include <QVariantList>

class QProcess;

/*!
 * Gestión nativa de dispositivos para el Centro de configuración:
 * impresoras (CUPS), Bluetooth (bluetoothctl), cámaras (V4L2) y discos
 * (lsblk + udisks2). Las operaciones se ejecutan con las herramientas del
 * sistema para no reinventar la pila de cada subsistema.
 */
class DevicesController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList printers READ printers NOTIFY printersChanged)
    Q_PROPERTY(QVariantList bluetoothDevices READ bluetoothDevices
                   NOTIFY bluetoothChanged)
    Q_PROPERTY(bool bluetoothAvailable READ bluetoothAvailable
                   NOTIFY bluetoothChanged)
    Q_PROPERTY(bool bluetoothEnabled READ bluetoothEnabled
                   NOTIFY bluetoothChanged)
    Q_PROPERTY(bool bluetoothScanning READ bluetoothScanning
                   NOTIFY bluetoothChanged)
    Q_PROPERTY(QVariantList cameras READ cameras NOTIFY camerasChanged)
    Q_PROPERTY(QVariantList disks READ disks NOTIFY disksChanged)

public:
    explicit DevicesController(QObject *parent = nullptr);

    QVariantList printers() const { return m_printers; }
    QVariantList bluetoothDevices() const { return m_bluetoothDevices; }
    bool bluetoothAvailable() const { return m_bluetoothAvailable; }
    bool bluetoothEnabled() const { return m_bluetoothEnabled; }
    bool bluetoothScanning() const { return m_bluetoothScanning; }
    QVariantList cameras() const { return m_cameras; }
    QVariantList disks() const { return m_disks; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void refreshPrinters();
    Q_INVOKABLE void refreshBluetooth();
    Q_INVOKABLE void refreshCameras();
    Q_INVOKABLE void refreshDisks();

    Q_INVOKABLE void setBluetoothEnabled(bool enabled);
    Q_INVOKABLE void scanBluetooth();
    Q_INVOKABLE void connectBluetooth(const QString &address);
    Q_INVOKABLE void disconnectBluetooth(const QString &address);

    Q_INVOKABLE bool mountDisk(const QString &device);
    Q_INVOKABLE bool unmountDisk(const QString &device);
    Q_INVOKABLE bool ejectDisk(const QString &device);

    Q_INVOKABLE void openPrinterSettings();
    Q_INVOKABLE void openBluetoothSettings();
    Q_INVOKABLE void openCamera(const QString &device);
    Q_INVOKABLE void openDiskUtility();

signals:
    void printersChanged();
    void bluetoothChanged();
    void camerasChanged();
    void disksChanged();
    void errorOccurred(const QString &message);

private:
    void runBluetoothCommand(const QStringList &arguments);
    void finishBluetoothScan();
    bool launchFirst(const QStringList &programs,
                     const QStringList &arguments = {});
    bool launch(const QString &program, const QStringList &arguments = {});

    QVariantList m_printers;
    QVariantList m_bluetoothDevices;
    bool m_bluetoothAvailable = false;
    bool m_bluetoothEnabled = false;
    bool m_bluetoothScanning = false;
    QVariantList m_cameras;
    QVariantList m_disks;
    QProcess *m_scanProcess = nullptr;
};
