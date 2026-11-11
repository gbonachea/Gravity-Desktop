#include "devicescontroller.h"

#include "launchenvironment.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>

#include <functional>

namespace
{

struct CommandResult {
    bool ok = false;
    QString output;
    QString error;
};

CommandResult runCommand(const QString &program, const QStringList &arguments,
                         int timeout = 5000)
{
    if (program.isEmpty()) {
        return {};
    }

    QProcess process;
    process.start(program, arguments);
    if (!process.waitForFinished(timeout)) {
        process.kill();
        process.waitForFinished(500);
        return {false, {}, QStringLiteral("timeout")};
    }

    CommandResult result;
    result.output = QString::fromLocal8Bit(process.readAllStandardOutput());
    result.error = QString::fromLocal8Bit(process.readAllStandardError());
    result.ok = process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
    return result;
}

QString readTextFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll()).trimmed();
}

} // namespace

DevicesController::DevicesController(QObject *parent)
    : QObject(parent)
{
    refresh();
}

void DevicesController::refresh()
{
    refreshPrinters();
    refreshBluetooth();
    refreshCameras();
    refreshDisks();
}

void DevicesController::refreshPrinters()
{
    const QString lpstat = QStandardPaths::findExecutable(QStringLiteral("lpstat"));
    if (lpstat.isEmpty()) {
        if (!m_printers.isEmpty()) {
            m_printers.clear();
            emit printersChanged();
        }
        return;
    }

    QString defaultPrinter;
    const CommandResult defaultResult = runCommand(lpstat, {QStringLiteral("-d")});
    if (defaultResult.ok) {
        const QString text = defaultResult.output.trimmed();
        const int colon = text.indexOf(QLatin1Char(':'));
        if (colon >= 0) {
            defaultPrinter = text.mid(colon + 1).trimmed();
        }
    }

    QVariantList printers;
    const CommandResult result = runCommand(lpstat, {QStringLiteral("-p")});
    const QRegularExpression pattern(
        QStringLiteral("^printer\\s+(\\S+)\\s+(.*)$"));
    for (const QString &line : result.output.split(QLatin1Char('\n'))) {
        const QRegularExpressionMatch match = pattern.match(line.trimmed());
        if (!match.hasMatch()) {
            continue;
        }
        const QString name = match.captured(1);
        QString status = match.captured(2).trimmed();
        status.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral(" "));
        printers.append(QVariantMap{
            {QStringLiteral("name"), name},
            {QStringLiteral("status"), status},
            {QStringLiteral("isDefault"), name == defaultPrinter},
        });
    }

    if (printers != m_printers) {
        m_printers = printers;
        emit printersChanged();
    }
}

void DevicesController::refreshBluetooth()
{
    const QString bluetoothctl =
        QStandardPaths::findExecutable(QStringLiteral("bluetoothctl"));

    bool available = false;
    bool enabled = false;
    QVariantList devices;

    if (!bluetoothctl.isEmpty()) {
        const CommandResult show = runCommand(bluetoothctl, {QStringLiteral("show")});
        available = !show.output.contains(QLatin1String("No default controller available"))
            && show.output.contains(QLatin1String("Controller "));
        const QRegularExpression powered(
            QStringLiteral("Powered:\\s*(yes|no)"));
        const QRegularExpressionMatch match = powered.match(show.output);
        enabled = match.hasMatch()
            && match.captured(1).compare(QLatin1String("yes"), Qt::CaseInsensitive) == 0;

        if (available) {
            QSet<QString> connected;
            const CommandResult connectedResult =
                runCommand(bluetoothctl, {QStringLiteral("devices"),
                                          QStringLiteral("Connected")});
            for (const QString &line : connectedResult.output.split(QLatin1Char('\n'))) {
                const QStringList parts = line.trimmed().split(QLatin1Char(' '));
                if (parts.size() >= 2 && parts.first() == QLatin1String("Device")) {
                    connected.insert(parts.at(1));
                }
            }

            const CommandResult list = runCommand(bluetoothctl, {QStringLiteral("devices")});
            const QRegularExpression devicePattern(
                QStringLiteral("^Device\\s+(\\S+)\\s+(.*)$"));
            for (const QString &line : list.output.split(QLatin1Char('\n'))) {
                const QRegularExpressionMatch deviceMatch =
                    devicePattern.match(line.trimmed());
                if (!deviceMatch.hasMatch()) {
                    continue;
                }
                const QString address = deviceMatch.captured(1);
                devices.append(QVariantMap{
                    {QStringLiteral("address"), address},
                    {QStringLiteral("name"), deviceMatch.captured(2).trimmed()},
                    {QStringLiteral("connected"), connected.contains(address)},
                });
            }
        }
    }

    if (available != m_bluetoothAvailable || enabled != m_bluetoothEnabled
        || devices != m_bluetoothDevices) {
        m_bluetoothAvailable = available;
        m_bluetoothEnabled = enabled;
        m_bluetoothDevices = devices;
        emit bluetoothChanged();
    }
}

void DevicesController::setBluetoothEnabled(bool enabled)
{
    const QString bluetoothctl =
        QStandardPaths::findExecutable(QStringLiteral("bluetoothctl"));
    if (bluetoothctl.isEmpty()) {
        emit errorOccurred(tr("No se encontró bluetoothctl (instala BlueZ)"));
        return;
    }
    const CommandResult result = runCommand(
        bluetoothctl, {QStringLiteral("power"), enabled ? QStringLiteral("on")
                                                        : QStringLiteral("off")});
    if (!result.ok) {
        emit errorOccurred(result.error.trimmed().isEmpty()
                               ? tr("No se pudo cambiar el estado de Bluetooth")
                               : result.error.trimmed());
    }
    refreshBluetooth();
}

void DevicesController::scanBluetooth()
{
    if (m_bluetoothScanning) {
        return;
    }
    const QString bluetoothctl =
        QStandardPaths::findExecutable(QStringLiteral("bluetoothctl"));
    if (bluetoothctl.isEmpty()) {
        emit errorOccurred(tr("No se encontró bluetoothctl (instala BlueZ)"));
        return;
    }

    if (m_scanProcess) {
        m_scanProcess->deleteLater();
    }
    m_scanProcess = new QProcess(this);
    connect(m_scanProcess, &QProcess::errorOccurred, this,
            [this](QProcess::ProcessError) {
                finishBluetoothScan();
            });
    connect(m_scanProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int, QProcess::ExitStatus) {
                finishBluetoothScan();
            });

    m_bluetoothScanning = true;
    emit bluetoothChanged();
    m_scanProcess->start(bluetoothctl, {QStringLiteral("--timeout"), QStringLiteral("8"),
                                        QStringLiteral("scan"), QStringLiteral("on")});
}

void DevicesController::finishBluetoothScan()
{
    m_bluetoothScanning = false;
    if (m_scanProcess) {
        m_scanProcess->deleteLater();
        m_scanProcess = nullptr;
    }
    refreshBluetooth();
    emit bluetoothChanged();
}

void DevicesController::runBluetoothCommand(const QStringList &arguments)
{
    const QString bluetoothctl =
        QStandardPaths::findExecutable(QStringLiteral("bluetoothctl"));
    if (bluetoothctl.isEmpty()) {
        emit errorOccurred(tr("No se encontró bluetoothctl (instala BlueZ)"));
        return;
    }
    const CommandResult result = runCommand(bluetoothctl, arguments);
    if (!result.ok) {
        emit errorOccurred(result.error.trimmed().isEmpty()
                               ? tr("La operación Bluetooth falló")
                               : result.error.trimmed());
    }
    refreshBluetooth();
}

void DevicesController::connectBluetooth(const QString &address)
{
    runBluetoothCommand({QStringLiteral("connect"), address});
}

void DevicesController::disconnectBluetooth(const QString &address)
{
    runBluetoothCommand({QStringLiteral("disconnect"), address});
}

void DevicesController::refreshCameras()
{
    QVariantList cameras;
    QSet<QString> names;

    const QDir directory(QStringLiteral("/sys/class/video4linux"));
    const QStringList entries =
        directory.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString &entry : entries) {
        const QString nameFile = directory.filePath(entry) + QStringLiteral("/name");
        QString name = readTextFile(nameFile);
        if (name.isEmpty()) {
            name = entry;
        }
        if (names.contains(name)) {
            continue;
        }
        names.insert(name);
        cameras.append(QVariantMap{
            {QStringLiteral("name"), name},
            {QStringLiteral("device"), QStringLiteral("/dev/") + entry},
        });
    }

    if (cameras != m_cameras) {
        m_cameras = cameras;
        emit camerasChanged();
    }
}

void DevicesController::refreshDisks()
{
    const QString lsblk = QStandardPaths::findExecutable(QStringLiteral("lsblk"));
    if (lsblk.isEmpty()) {
        if (!m_disks.isEmpty()) {
            m_disks.clear();
            emit disksChanged();
        }
        return;
    }

    const CommandResult result = runCommand(
        lsblk, {QStringLiteral("-J"), QStringLiteral("-o"),
                QStringLiteral("NAME,SIZE,TYPE,MOUNTPOINT,LABEL,MODEL,RM,FSTYPE")});

    QVariantList disks;
    if (result.ok) {
        const QJsonDocument document = QJsonDocument::fromJson(result.output.toUtf8());
        const QJsonArray devices =
            document.object().value(QStringLiteral("blockdevices")).toArray();

        std::function<void(const QJsonObject &)> addDevice =
            [&](const QJsonObject &device) {
                const QString type = device.value(QStringLiteral("type")).toString();
                const QString name = device.value(QStringLiteral("name")).toString();
                const QString fstype = device.value(QStringLiteral("fstype")).toString();
                const QJsonArray children =
                    device.value(QStringLiteral("children")).toArray();

                const bool accepted = (type == QLatin1String("disk")
                                       || type == QLatin1String("part"))
                    && !name.startsWith(QLatin1String("loop"))
                    && !name.startsWith(QLatin1String("zram"))
                    && !(type == QLatin1String("disk") && !children.isEmpty()
                         && fstype.isEmpty());

                if (accepted) {
                    disks.append(QVariantMap{
                        {QStringLiteral("name"), QStringLiteral("/dev/") + name},
                        {QStringLiteral("shortName"), name},
                        {QStringLiteral("size"), device.value(QStringLiteral("size")).toString()},
                        {QStringLiteral("label"), device.value(QStringLiteral("label")).toString()},
                        {QStringLiteral("model"), device.value(QStringLiteral("model")).toString()},
                        {QStringLiteral("mountpoint"), device.value(QStringLiteral("mountpoint")).toString()},
                        {QStringLiteral("fstype"), fstype},
                        {QStringLiteral("removable"), device.value(QStringLiteral("rm")).toBool()},
                        {QStringLiteral("type"), type},
                    });
                }

                for (const QJsonValue &child : children) {
                    addDevice(child.toObject());
                }
            };

        for (const QJsonValue &value : devices) {
            addDevice(value.toObject());
        }
    }

    if (disks != m_disks) {
        m_disks = disks;
        emit disksChanged();
    }
}

bool DevicesController::mountDisk(const QString &device)
{
    const QString udisksctl =
        QStandardPaths::findExecutable(QStringLiteral("udisksctl"));
    if (udisksctl.isEmpty()) {
        emit errorOccurred(tr("No se encontró udisksctl (instala udisks2)"));
        return false;
    }
    const CommandResult result =
        runCommand(udisksctl, {QStringLiteral("mount"), QStringLiteral("-b"), device});
    if (!result.ok) {
        emit errorOccurred(result.error.trimmed().isEmpty()
                               ? tr("No se pudo montar %1").arg(device)
                               : result.error.trimmed());
    }
    refreshDisks();
    return result.ok;
}

bool DevicesController::unmountDisk(const QString &device)
{
    const QString udisksctl =
        QStandardPaths::findExecutable(QStringLiteral("udisksctl"));
    if (udisksctl.isEmpty()) {
        emit errorOccurred(tr("No se encontró udisksctl (instala udisks2)"));
        return false;
    }
    const CommandResult result =
        runCommand(udisksctl, {QStringLiteral("unmount"), QStringLiteral("-b"), device});
    if (!result.ok) {
        emit errorOccurred(result.error.trimmed().isEmpty()
                               ? tr("No se pudo desmontar %1").arg(device)
                               : result.error.trimmed());
    }
    refreshDisks();
    return result.ok;
}

bool DevicesController::ejectDisk(const QString &device)
{
    const QString udisksctl =
        QStandardPaths::findExecutable(QStringLiteral("udisksctl"));
    if (udisksctl.isEmpty()) {
        emit errorOccurred(tr("No se encontró udisksctl (instala udisks2)"));
        return false;
    }
    const CommandResult result =
        runCommand(udisksctl, {QStringLiteral("power-off"), QStringLiteral("-b"), device});
    if (!result.ok) {
        emit errorOccurred(result.error.trimmed().isEmpty()
                               ? tr("No se pudo expulsar %1").arg(device)
                               : result.error.trimmed());
    }
    refreshDisks();
    return result.ok;
}

void DevicesController::openPrinterSettings()
{
    const QStringList direct = {
        QStringLiteral("system-config-printer"),
        QStringLiteral("kcmshell6"),
        QStringLiteral("kcmshell5"),
    };
    for (const QString &program : direct) {
        const QString path = QStandardPaths::findExecutable(program);
        if (path.isEmpty()) {
            continue;
        }
        if (program.startsWith(QLatin1String("kcmshell"))) {
            if (launch(path, {QStringLiteral("printer")})) {
                return;
            }
        } else if (launch(path)) {
            return;
        }
    }

    const QString gnome = QStandardPaths::findExecutable(QStringLiteral("gnome-control-center"));
    if (!gnome.isEmpty() && launch(gnome, {QStringLiteral("printers")})) {
        return;
    }

    if (launchFirst({QStringLiteral("systemsettings6"), QStringLiteral("systemsettings5"),
                     QStringLiteral("systemsettings"), QStringLiteral("stellar-settings")})) {
        return;
    }
    emit errorOccurred(tr("No se encontró una aplicación de configuración de impresoras"));
}

void DevicesController::openBluetoothSettings()
{
    if (launchFirst({QStringLiteral("blueman-manager")})) {
        return;
    }
    const QString gnome = QStandardPaths::findExecutable(QStringLiteral("gnome-control-center"));
    if (!gnome.isEmpty() && launch(gnome, {QStringLiteral("bluetooth")})) {
        return;
    }
    if (launchFirst({QStringLiteral("systemsettings6"), QStringLiteral("systemsettings5"),
                     QStringLiteral("systemsettings"), QStringLiteral("stellar-settings")})) {
        return;
    }
    emit errorOccurred(tr("No se encontró una aplicación de configuración de Bluetooth"));
}

void DevicesController::openCamera(const QString &device)
{
    const QStringList cameras = {
        QStringLiteral("cheese"),
        QStringLiteral("guvcview"),
        QStringLiteral("kamoso"),
        QStringLiteral("snapshot"),
    };
    for (const QString &program : cameras) {
        const QString path = QStandardPaths::findExecutable(program);
        if (!path.isEmpty() && launch(path)) {
            return;
        }
    }

    const QString mpv = QStandardPaths::findExecutable(QStringLiteral("mpv"));
    if (!mpv.isEmpty()
        && launch(mpv, {QStringLiteral("av://v4l2:") + device,
                        QStringLiteral("--profile=low-latency")})) {
        return;
    }
    emit errorOccurred(tr("No se encontró una aplicación para abrir la cámara"));
}

void DevicesController::openDiskUtility()
{
    if (launchFirst({QStringLiteral("gnome-disks"),
                     QStringLiteral("partitionmanager"),
                     QStringLiteral("gparted"),
                     QStringLiteral("systemsettings6"),
                     QStringLiteral("systemsettings5"),
                     QStringLiteral("stellar-settings")})) {
        return;
    }
    emit errorOccurred(tr("No se encontró una aplicación de discos"));
}

bool DevicesController::launchFirst(const QStringList &programs,
                                    const QStringList &arguments)
{
    for (const QString &program : programs) {
        const QString path = QStandardPaths::findExecutable(program);
        if (!path.isEmpty() && launch(path, arguments)) {
            return true;
        }
    }
    return false;
}

bool DevicesController::launch(const QString &program, const QStringList &arguments)
{
    if (program.isEmpty()) {
        return false;
    }
    return Stellar::startDetached(program, arguments, QDir::homePath());
}
