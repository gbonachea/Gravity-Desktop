#include "settingscontroller.h"

#include "desktopentrymodel.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QProcess>
#include <QStandardPaths>
#include <QSysInfo>
#include <QTextStream>

namespace
{
QString readFileValue(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromLatin1(file.readAll()).trimmed();
}

QStringList defaultWidgetOrder()
{
    return {QStringLiteral("clock"), QStringLiteral("storage"), QStringLiteral("status")};
}
} // namespace

SettingsController::SettingsController(QObject *parent)
    : QObject(parent)
    , m_settings(QStringLiteral("Stellar"), QStringLiteral("StellarShell"))
{
    // Asegura que Qt encuentre el tema de iconos Bubble instalado por el paquete.
    QStringList paths = QIcon::themeSearchPaths();
    const QStringList extra = {
        QStringLiteral("/usr/share/icons"),
        QDir::homePath() + QStringLiteral("/.local/share/icons"),
        QDir::homePath() + QStringLiteral("/.icons"),
    };
    for (const QString &path : extra) {
        if (!paths.contains(path)) {
            paths.append(path);
        }
    }
    QIcon::setThemeSearchPaths(paths);

    // Migración: instalaciones antiguas guardaban "Adwaita"/"Bubble" o nada.
    QString theme = m_settings.value(QStringLiteral("iconTheme")).toString();
    if (theme.isEmpty() || theme == QLatin1String("Adwaita")
        || theme == QLatin1String("Bubble")) {
        theme = QStringLiteral("DamaDamas");
        m_settings.setValue(QStringLiteral("iconTheme"), theme);
    }
    QString cursor = m_settings.value(QStringLiteral("cursorTheme")).toString();
    if (cursor.isEmpty() || cursor == QLatin1String("Adwaita")) {
        cursor = QStringLiteral("breeze_cursors");
        m_settings.setValue(QStringLiteral("cursorTheme"), cursor);
    }

    QIcon::setThemeName(theme);
    QIcon::setFallbackThemeName(QStringLiteral("DamaDamas"));
}

void SettingsController::reloadSettings()
{
    // QSettings cachea los valores; sync() recarga los cambios hechos por
    // otros procesos (por ejemplo, el Centro de configuración).
    m_settings.sync();
}

QString SettingsController::settingsFilePath() const
{
    return m_settings.fileName();
}

// ---------------------------------------------------------------------------
// Panel
// ---------------------------------------------------------------------------

int SettingsController::panelHeight() const
{
    return m_settings.value(QStringLiteral("panelHeight"), 64).toInt();
}

void SettingsController::setPanelHeight(int value)
{
    value = qBound(48, value, 96);
    if (value == panelHeight()) {
        return;
    }
    m_settings.setValue(QStringLiteral("panelHeight"), value);
    emit panelHeightChanged();
}

QString SettingsController::panelPosition() const
{
    return m_settings.value(QStringLiteral("panelPosition"), QStringLiteral("bottom")).toString();
}

void SettingsController::setPanelPosition(const QString &value)
{
    if (value != QLatin1String("top") && value != QLatin1String("bottom")) {
        return;
    }
    if (value == panelPosition()) {
        return;
    }
    m_settings.setValue(QStringLiteral("panelPosition"), value);
    emit panelPositionChanged();
}

bool SettingsController::panelAutoHide() const
{
    return m_settings.value(QStringLiteral("panelAutoHide"), false).toBool();
}

void SettingsController::setPanelAutoHide(bool value)
{
    if (value == panelAutoHide()) {
        return;
    }
    m_settings.setValue(QStringLiteral("panelAutoHide"), value);
    emit panelAutoHideChanged();
}

int SettingsController::panelOpacity() const
{
    return m_settings.value(QStringLiteral("panelOpacity"), 100).toInt();
}

void SettingsController::setPanelOpacity(int value)
{
    value = qBound(30, value, 100);
    if (value == panelOpacity()) {
        return;
    }
    m_settings.setValue(QStringLiteral("panelOpacity"), value);
    emit panelOpacityChanged();
}

// ---------------------------------------------------------------------------
// Menú
// ---------------------------------------------------------------------------

bool SettingsController::menuShowSearch() const
{
    return m_settings.value(QStringLiteral("menuShowSearch"), true).toBool();
}

void SettingsController::setMenuShowSearch(bool value)
{
    if (value == menuShowSearch()) {
        return;
    }
    m_settings.setValue(QStringLiteral("menuShowSearch"), value);
    emit menuShowSearchChanged();
}

bool SettingsController::menuShowRecent() const
{
    return m_settings.value(QStringLiteral("menuShowRecent"), true).toBool();
}

void SettingsController::setMenuShowRecent(bool value)
{
    if (value == menuShowRecent()) {
        return;
    }
    m_settings.setValue(QStringLiteral("menuShowRecent"), value);
    emit menuShowRecentChanged();
}

QStringList SettingsController::menuFavorites() const
{
    // Una lista vacía guardada explícitamente significa "sin favoritos"; solo
    // cuando la clave no existe se ofrecen los favoritos predeterminados.
    if (!m_settings.contains(QStringLiteral("menuFavorites"))) {
        return defaultMenuFavorites();
    }
    return m_settings.value(QStringLiteral("menuFavorites")).toStringList();
}

QStringList SettingsController::defaultMenuFavorites()
{
    const QStringList preferred = {
        QStringLiteral("stellar-settings.desktop"),
        QStringLiteral("org.kde.konsole.desktop"),
        QStringLiteral("org.kde.dolphin.desktop"),
    };

    QStringList result;
    const auto &entries = DesktopEntryIndex::instance().entries();
    for (const QString &file : preferred) {
        for (const DesktopEntry &entry : entries) {
            if (entry.fileName == file) {
                result.append(file);
                break;
            }
        }
    }
    return result;
}

void SettingsController::setMenuFavorites(const QStringList &value)
{
    if (value == menuFavorites()) {
        return;
    }
    m_settings.setValue(QStringLiteral("menuFavorites"), value);
    emit menuFavoritesChanged();
}

QVariantList SettingsController::appChoices() const
{
    const QStringList favorites = menuFavorites();
    QVariantList result;
    for (const DesktopEntry &entry : DesktopEntryIndex::instance().entries()) {
        QVariantMap item;
        item.insert(QStringLiteral("file"), entry.fileName);
        item.insert(QStringLiteral("name"), entry.name);
        item.insert(QStringLiteral("icon"), entry.icon.isEmpty()
                                               ? QStringLiteral("application-x-executable")
                                               : entry.icon);
        item.insert(QStringLiteral("favorite"), favorites.contains(entry.fileName));
        result.append(item);
    }
    return result;
}

bool SettingsController::isFavorite(const QString &file) const
{
    return menuFavorites().contains(file);
}

void SettingsController::toggleFavorite(const QString &file)
{
    if (file.isEmpty()) {
        return;
    }
    QStringList favorites = menuFavorites();
    if (favorites.contains(file)) {
        favorites.removeAll(file);
    } else {
        favorites.append(file);
    }
    setMenuFavorites(favorites);
}

void SettingsController::moveFavorite(const QString &file, int delta)
{
    QStringList favorites = menuFavorites();
    const int index = favorites.indexOf(file);
    const int target = index + delta;
    if (index < 0 || target < 0 || target >= favorites.size()) {
        return;
    }
    favorites.move(index, target);
    setMenuFavorites(favorites);
}

QString SettingsController::appName(const QString &file) const
{
    for (const DesktopEntry &entry : DesktopEntryIndex::instance().entries()) {
        if (entry.fileName == file) {
            return entry.name;
        }
    }
    return file;
}

QString SettingsController::appIcon(const QString &file) const
{
    for (const DesktopEntry &entry : DesktopEntryIndex::instance().entries()) {
        if (entry.fileName == file) {
            return entry.icon.isEmpty() ? QStringLiteral("application-x-executable")
                                        : entry.icon;
        }
    }
    return QStringLiteral("application-x-executable");
}

// ---------------------------------------------------------------------------
// Widgets
// ---------------------------------------------------------------------------

bool SettingsController::widgetClock() const
{
    return m_settings.value(QStringLiteral("widgetClock"), true).toBool();
}

void SettingsController::setWidgetClock(bool value)
{
    if (value == widgetClock()) {
        return;
    }
    m_settings.setValue(QStringLiteral("widgetClock"), value);
    emit widgetsChanged();
}

bool SettingsController::widgetStorage() const
{
    return m_settings.value(QStringLiteral("widgetStorage"), true).toBool();
}

void SettingsController::setWidgetStorage(bool value)
{
    if (value == widgetStorage()) {
        return;
    }
    m_settings.setValue(QStringLiteral("widgetStorage"), value);
    emit widgetsChanged();
}

bool SettingsController::widgetStatus() const
{
    return m_settings.value(QStringLiteral("widgetStatus"), true).toBool();
}

void SettingsController::setWidgetStatus(bool value)
{
    if (value == widgetStatus()) {
        return;
    }
    m_settings.setValue(QStringLiteral("widgetStatus"), value);
    emit widgetsChanged();
}

QStringList SettingsController::widgetOrder() const
{
    const QStringList stored =
        m_settings.value(QStringLiteral("widgetOrder")).toStringList();
    if (stored.isEmpty()) {
        return defaultWidgetOrder();
    }
    // Sanea: solo nombres conocidos y sin duplicados.
    QStringList result;
    for (const QString &name : stored) {
        if ((name == QLatin1String("clock") || name == QLatin1String("storage")
             || name == QLatin1String("status"))
            && !result.contains(name)) {
            result.append(name);
        }
    }
    for (const QString &name : defaultWidgetOrder()) {
        if (!result.contains(name)) {
            result.append(name);
        }
    }
    return result;
}

void SettingsController::setWidgetOrder(const QStringList &value)
{
    if (value == widgetOrder()) {
        return;
    }
    m_settings.setValue(QStringLiteral("widgetOrder"), value);
    emit widgetsChanged();
}

QStringList SettingsController::visibleWidgets() const
{
    QStringList result;
    for (const QString &name : widgetOrder()) {
        const bool enabled = name == QLatin1String("clock")   ? widgetClock()
            : name == QLatin1String("storage")               ? widgetStorage()
            : name == QLatin1String("status")                ? widgetStatus()
                                                             : false;
        if (enabled) {
            result.append(name);
        }
    }
    return result;
}

void SettingsController::moveWidget(const QString &name, int delta)
{
    QStringList order = widgetOrder();
    const int index = order.indexOf(name);
    const int target = index + delta;
    if (index < 0 || target < 0 || target >= order.size()) {
        return;
    }
    order.move(index, target);
    setWidgetOrder(order);
}

QString SettingsController::widgetLabel(const QString &name) const
{
    if (name == QLatin1String("clock")) {
        return tr("Reloj y clima");
    }
    if (name == QLatin1String("storage")) {
        return tr("Almacenamiento");
    }
    if (name == QLatin1String("status")) {
        return tr("Estado del sistema");
    }
    return name;
}

bool SettingsController::weatherAutoLocate() const
{
    return m_settings.value(QStringLiteral("weatherAutoLocate"), true).toBool();
}

void SettingsController::setWeatherAutoLocate(bool value)
{
    if (value == weatherAutoLocate()) {
        return;
    }
    m_settings.setValue(QStringLiteral("weatherAutoLocate"), value);
    emit weatherLocationChanged();
}

QString SettingsController::weatherCity() const
{
    return m_settings.value(QStringLiteral("weatherCity"),
                            QStringLiteral("Madrid")).toString();
}

void SettingsController::setWeatherCity(const QString &value)
{
    if (value == weatherCity()) {
        return;
    }
    m_settings.setValue(QStringLiteral("weatherCity"), value.trimmed());
    emit weatherLocationChanged();
}

// ---------------------------------------------------------------------------
// Apariencia
// ---------------------------------------------------------------------------

QString SettingsController::wallpaper() const
{
    return m_settings.value(QStringLiteral("wallpaper")).toString();
}

void SettingsController::setWallpaper(const QString &value)
{
    if (value == wallpaper()) {
        return;
    }
    m_settings.setValue(QStringLiteral("wallpaper"), value);
    emit wallpaperChanged();
}

QString SettingsController::iconTheme() const
{
    return m_settings.value(QStringLiteral("iconTheme"),
                            QStringLiteral("DamaDamas")).toString();
}

void SettingsController::setIconTheme(const QString &value)
{
    if (value == iconTheme()) {
        return;
    }
    m_settings.setValue(QStringLiteral("iconTheme"), value);
    QIcon::setThemeName(value);
    QIcon::setFallbackThemeName(QStringLiteral("DamaDamas"));
    gsettings(QStringLiteral("org.gnome.desktop.interface"),
              QStringLiteral("icon-theme"), value);
    emit iconThemeChanged();
}

QString SettingsController::cursorTheme() const
{
    return m_settings.value(QStringLiteral("cursorTheme"),
                            QStringLiteral("breeze_cursors")).toString();
}

void SettingsController::setCursorTheme(const QString &value)
{
    if (value == cursorTheme()) {
        return;
    }
    m_settings.setValue(QStringLiteral("cursorTheme"), value);
    gsettings(QStringLiteral("org.gnome.desktop.interface"),
              QStringLiteral("cursor-theme"), value);
    emit cursorThemeChanged();
}

int SettingsController::cursorSize() const
{
    return m_settings.value(QStringLiteral("cursorSize"), 24).toInt();
}

void SettingsController::setCursorSize(int value)
{
    value = qBound(16, value, 64);
    if (value == cursorSize()) {
        return;
    }
    m_settings.setValue(QStringLiteral("cursorSize"), value);
    gsettings(QStringLiteral("org.gnome.desktop.interface"),
              QStringLiteral("cursor-size"), QString::number(value));
    emit cursorSizeChanged();
}

bool SettingsController::desktopShowComputer() const
{
    return m_settings.value(QStringLiteral("desktopShowComputer"), true).toBool();
}

void SettingsController::setDesktopShowComputer(bool value)
{
    if (value == desktopShowComputer()) {
        return;
    }
    m_settings.setValue(QStringLiteral("desktopShowComputer"), value);
    applyDesktopIcons();
    emit desktopIconsChanged();
}

bool SettingsController::desktopShowUser() const
{
    return m_settings.value(QStringLiteral("desktopShowUser"), true).toBool();
}

void SettingsController::setDesktopShowUser(bool value)
{
    if (value == desktopShowUser()) {
        return;
    }
    m_settings.setValue(QStringLiteral("desktopShowUser"), value);
    applyDesktopIcons();
    emit desktopIconsChanged();
}

bool SettingsController::desktopShowTrash() const
{
    return m_settings.value(QStringLiteral("desktopShowTrash"), true).toBool();
}

void SettingsController::setDesktopShowTrash(bool value)
{
    if (value == desktopShowTrash()) {
        return;
    }
    m_settings.setValue(QStringLiteral("desktopShowTrash"), value);
    applyDesktopIcons();
    emit desktopIconsChanged();
}

bool SettingsController::notificationsMuted() const
{
    return m_settings.value(QStringLiteral("notificationsMuted"), false).toBool();
}

void SettingsController::setNotificationsMuted(bool value)
{
    if (value == notificationsMuted()) {
        return;
    }
    m_settings.setValue(QStringLiteral("notificationsMuted"), value);
    emit notificationsSettingsChanged();
}

bool SettingsController::notificationsTrayIcon() const
{
    return m_settings.value(QStringLiteral("notificationsTrayIcon"), true).toBool();
}

void SettingsController::setNotificationsTrayIcon(bool value)
{
    if (value == notificationsTrayIcon()) {
        return;
    }
    m_settings.setValue(QStringLiteral("notificationsTrayIcon"), value);
    emit notificationsSettingsChanged();
}

bool SettingsController::notificationsPopup() const
{
    return m_settings.value(QStringLiteral("notificationsPopup"), true).toBool();
}

void SettingsController::setNotificationsPopup(bool value)
{
    if (value == notificationsPopup()) {
        return;
    }
    m_settings.setValue(QStringLiteral("notificationsPopup"), value);
    emit notificationsSettingsChanged();
}

QString SettingsController::desktopDir() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    if (dir.isEmpty()) {
        dir = QDir::homePath() + QStringLiteral("/Desktop");
    }
    QDir().mkpath(dir);
    return dir;
}

bool SettingsController::writeDesktopEntry(const QString &fileName, const QString &name,
                                           const QString &exec, const QString &icon)
{
    const QString path = desktopDir() + QLatin1Char('/') + fileName;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return false;
    }
    QTextStream out(&file);
    out << "[Desktop Entry]\n";
    out << "Type=Application\n";
    out << "Name=" << name << "\n";
    out << "Comment=Acceso de Stellar Shell\n";
    out << "Exec=" << exec << "\n";
    out << "Icon=" << icon << "\n";
    out << "Terminal=false\n";
    out << "Categories=Utility;\n";
    out << "X-Stellar-DesktopIcon=true\n";
    file.close();
    QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner
                                    | QFile::ReadGroup | QFile::ReadOther);
    return true;
}

bool SettingsController::removeDesktopEntry(const QString &fileName)
{
    const QString path = desktopDir() + QLatin1Char('/') + fileName;
    return QFile::remove(path);
}

void SettingsController::applyDesktopIcons()
{
    if (desktopShowComputer()) {
        writeDesktopEntry(QStringLiteral("stellar-equipo.desktop"), tr("Equipo"),
                          QStringLiteral("sh -c \"xdg-open computer:/// || xdg-open /\""),
                          QStringLiteral("computer"));
    } else {
        removeDesktopEntry(QStringLiteral("stellar-equipo.desktop"));
    }

    if (desktopShowUser()) {
        writeDesktopEntry(QStringLiteral("stellar-usuario.desktop"), tr("Usuario"),
                          QStringLiteral("xdg-open %1").arg(QDir::homePath()),
                          QStringLiteral("user-home"));
    } else {
        removeDesktopEntry(QStringLiteral("stellar-usuario.desktop"));
    }

    if (desktopShowTrash()) {
        writeDesktopEntry(QStringLiteral("stellar-papelera.desktop"), tr("Papelera"),
                          QStringLiteral("xdg-open trash:///"),
                          QStringLiteral("user-trash"));
    } else {
        removeDesktopEntry(QStringLiteral("stellar-papelera.desktop"));
    }
}

bool SettingsController::pinToDesktop(const QString &desktopFile)
{
    if (desktopFile.isEmpty()) {
        return false;
    }
    for (const DesktopEntry &entry : DesktopEntryIndex::instance().entries()) {
        if (entry.fileName != desktopFile) {
            continue;
        }
        return writeDesktopEntry(entry.fileName,
                                 entry.name,
                                 entry.exec,
                                 entry.icon.isEmpty()
                                     ? QStringLiteral("application-x-executable")
                                     : entry.icon);
    }
    return false;
}

QString SettingsController::colorScheme() const
{
    return m_settings.value(QStringLiteral("colorScheme"), QStringLiteral("default")).toString();
}

void SettingsController::setColorScheme(const QString &value)
{
    if (value == colorScheme()) {
        return;
    }
    m_settings.setValue(QStringLiteral("colorScheme"), value);
    gsettings(QStringLiteral("org.gnome.desktop.interface"),
              QStringLiteral("color-scheme"), value);
    emit colorSchemeChanged();
}

// ---------------------------------------------------------------------------
// Pantalla
// ---------------------------------------------------------------------------

int SettingsController::brightness() const
{
    const QString maxStr = readFileValue(
        QStringLiteral("/sys/class/backlight/amdgpu_bl1/max_brightness"));
    const QString curStr = readFileValue(
        QStringLiteral("/sys/class/backlight/amdgpu_bl1/brightness"));
    bool okMax = false, okCur = false;
    const int max = maxStr.toInt(&okMax);
    const int cur = curStr.toInt(&okCur);
    if (!okMax || max <= 0 || !okCur) {
        return -1;
    }
    return qBound(0, qRound(cur * 100.0 / max), 100);
}

void SettingsController::setBrightness(int percent)
{
    percent = qBound(0, percent, 100);
    const QString maxStr = readFileValue(
        QStringLiteral("/sys/class/backlight/amdgpu_bl1/max_brightness"));
    bool okMax = false;
    const int max = maxStr.toInt(&okMax);
    if (!okMax || max <= 0) {
        return;
    }
    const int value = qRound(percent * max / 100.0);
    const QString path = QStringLiteral("/sys/class/backlight/amdgpu_bl1/brightness");
    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(QByteArray::number(value));
        file.close();
        emit brightnessChanged();
        return;
    }
    QProcess process;
    process.start(QStringLiteral("pkexec"), {QStringLiteral("tee"), path});
    if (process.waitForStarted(3000)) {
        process.write(QByteArray::number(value) + '\n');
        process.closeWriteChannel();
        process.waitForFinished(5000);
        emit brightnessChanged();
    }
}

// ---------------------------------------------------------------------------
// Teclado
// ---------------------------------------------------------------------------

int SettingsController::repeatDelay() const
{
    return m_settings.value(QStringLiteral("repeatDelay"), 500).toInt();
}

void SettingsController::setRepeatDelay(int value)
{
    value = qBound(100, value, 1000);
    if (value == repeatDelay()) {
        return;
    }
    m_settings.setValue(QStringLiteral("repeatDelay"), value);
    gsettings(QStringLiteral("org.gnome.desktop.peripherals.keyboard"),
              QStringLiteral("repeat-delay"), QString::number(value));
    emit repeatDelayChanged();
}

int SettingsController::repeatRate() const
{
    return m_settings.value(QStringLiteral("repeatRate"), 25).toInt();
}

void SettingsController::setRepeatRate(int value)
{
    value = qBound(10, value, 100);
    if (value == repeatRate()) {
        return;
    }
    m_settings.setValue(QStringLiteral("repeatRate"), value);
    gsettings(QStringLiteral("org.gnome.desktop.peripherals.keyboard"),
              QStringLiteral("repeat-interval"), QString::number(value));
    emit repeatRateChanged();
}

// ---------------------------------------------------------------------------
// Ratón / touchpad
// ---------------------------------------------------------------------------

double SettingsController::mouseSpeed() const
{
    return m_settings.value(QStringLiteral("mouseSpeed"), 0.0).toDouble();
}

void SettingsController::setMouseSpeed(double value)
{
    value = qBound(-1.0, value, 1.0);
    if (qFuzzyCompare(value + 1.0, mouseSpeed() + 1.0)) {
        return;
    }
    m_settings.setValue(QStringLiteral("mouseSpeed"), value);
    gsettings(QStringLiteral("org.gnome.desktop.peripherals.mouse"),
              QStringLiteral("speed"), QString::number(value));
    emit mouseSpeedChanged();
}

bool SettingsController::mouseNaturalScroll() const
{
    return m_settings.value(QStringLiteral("mouseNaturalScroll"), false).toBool();
}

void SettingsController::setMouseNaturalScroll(bool value)
{
    if (value == mouseNaturalScroll()) {
        return;
    }
    m_settings.setValue(QStringLiteral("mouseNaturalScroll"), value);
    gsettings(QStringLiteral("org.gnome.desktop.peripherals.mouse"),
              QStringLiteral("natural-scroll"), value ? QStringLiteral("true")
                                                      : QStringLiteral("false"));
    emit mouseNaturalScrollChanged();
}

bool SettingsController::touchpadEnabled() const
{
    return m_settings.value(QStringLiteral("touchpadEnabled"), true).toBool();
}

void SettingsController::setTouchpadEnabled(bool value)
{
    if (value == touchpadEnabled()) {
        return;
    }
    m_settings.setValue(QStringLiteral("touchpadEnabled"), value);
    gsettings(QStringLiteral("org.gnome.desktop.peripherals.touchpad"),
              QStringLiteral("send-events"),
              value ? QStringLiteral("enabled") : QStringLiteral("disabled"));
    emit touchpadEnabledChanged();
}

bool SettingsController::touchpadTapToClick() const
{
    return m_settings.value(QStringLiteral("touchpadTapToClick"), true).toBool();
}

void SettingsController::setTouchpadTapToClick(bool value)
{
    if (value == touchpadTapToClick()) {
        return;
    }
    m_settings.setValue(QStringLiteral("touchpadTapToClick"), value);
    gsettings(QStringLiteral("org.gnome.desktop.peripherals.touchpad"),
              QStringLiteral("tap-to-click"), value ? QStringLiteral("true")
                                                    : QStringLiteral("false"));
    emit touchpadTapToClickChanged();
}

bool SettingsController::touchpadNaturalScroll() const
{
    return m_settings.value(QStringLiteral("touchpadNaturalScroll"), false).toBool();
}

void SettingsController::setTouchpadNaturalScroll(bool value)
{
    if (value == touchpadNaturalScroll()) {
        return;
    }
    m_settings.setValue(QStringLiteral("touchpadNaturalScroll"), value);
    gsettings(QStringLiteral("org.gnome.desktop.peripherals.touchpad"),
              QStringLiteral("natural-scroll"), value ? QStringLiteral("true")
                                                      : QStringLiteral("false"));
    emit touchpadNaturalScrollChanged();
}

// ---------------------------------------------------------------------------
// Utilidades
// ---------------------------------------------------------------------------

void SettingsController::gsettings(const QString &schema, const QString &key,
                                   const QString &value)
{
    QProcess::startDetached(QStringLiteral("gsettings"),
                           {QStringLiteral("set"), schema, key, value});
}

QString SettingsController::runCommand(const QString &program, const QStringList &args)
{
    QProcess process;
    process.start(program, args);
    if (!process.waitForFinished(8000)) {
        return QStringLiteral("timeout");
    }
    return QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed()
        + QString::fromLocal8Bit(process.readAllStandardError()).trimmed();
}

QStringList SettingsController::listIconThemes()
{
    QStringList result;
    const QStringList roots = {
        QStringLiteral("/usr/share/icons"),
        QDir::homePath() + QStringLiteral("/.icons"),
        QDir::homePath() + QStringLiteral("/.local/share/icons"),
    };
    for (const QString &root : roots) {
        QDir dir(root);
        if (!dir.exists()) {
            continue;
        }
        for (const QString &entry : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            const QString index = dir.filePath(entry) + QStringLiteral("/index.theme");
            if (QFileInfo::exists(index) && !result.contains(entry)) {
                result.append(entry);
            }
        }
    }
    result.sort();
    return result;
}

QStringList SettingsController::listCursorThemes()
{
    return listIconThemes();
}

QStringList SettingsController::listDisplayOutputs()
{
    const QString output = runCommand(QStringLiteral("xrandr"), {});
    QStringList result;
    for (const QString &line : output.split(QLatin1Char('\n'))) {
        if (line.contains(QLatin1String(" connected"))) {
            const QString name = line.section(QLatin1Char(' '), 0, 0);
            if (!name.isEmpty()) {
                result.append(name);
            }
        }
    }
    return result;
}

bool SettingsController::setDisplayMode(const QString &output, const QString &mode)
{
    if (output.isEmpty() || mode.isEmpty()) {
        return false;
    }
    QProcess process;
    process.start(QStringLiteral("xrandr"),
                 {QStringLiteral("--output"), output, QStringLiteral("--mode"), mode});
    return process.waitForFinished(8000) && process.exitCode() == 0;
}

QString SettingsController::currentDateTime()
{
    return QDateTime::currentDateTime().toString(QStringLiteral("dddd, d MMMM yyyy  hh:mm"));
}

void SettingsController::setAutoTime(bool enabled)
{
    QProcess::startDetached(QStringLiteral("timedatectl"),
                           {QStringLiteral("set-ntp"), enabled ? QStringLiteral("true")
                                                              : QStringLiteral("false")});
}

void SettingsController::setTimeZone(const QString &zone)
{
    if (zone.isEmpty()) {
        return;
    }
    QProcess::startDetached(QStringLiteral("timedatectl"),
                           {QStringLiteral("set-timezone"), zone});
}

void SettingsController::setKeyboardLayout(const QString &layout)
{
    if (layout.isEmpty()) {
        return;
    }
    QProcess::startDetached(QStringLiteral("setxkbmap"), {layout});
    gsettings(QStringLiteral("org.gnome.desktop.input-sources"),
              QStringLiteral("sources"),
              QStringLiteral("[('xkb', '%1')]").arg(layout));
}

QStringList SettingsController::powerProfiles()
{
    const QString output = runCommand(QStringLiteral("powerprofilesctl"),
                                     {QStringLiteral("list")});
    QStringList result;
    for (const QString &line : output.split(QLatin1Char('\n'))) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }
        trimmed.remove(QChar('*'));
        trimmed = trimmed.trimmed();
        if (!trimmed.isEmpty() && !result.contains(trimmed)) {
            result.append(trimmed);
        }
    }
    return result;
}

void SettingsController::setPowerProfile(const QString &profile)
{
    if (profile.isEmpty()) {
        return;
    }
    QProcess::startDetached(QStringLiteral("powerprofilesctl"),
                           {QStringLiteral("set"), profile});
}

QString SettingsController::currentPowerProfile()
{
    return runCommand(QStringLiteral("powerprofilesctl"), {QStringLiteral("get")}).trimmed();
}

QStringList SettingsController::listFonts()
{
    const QString output = runCommand(QStringLiteral("fc-list"),
                                     {QStringLiteral(":family")});
    QStringList result;
    for (const QString &line : output.split(QLatin1Char('\n'))) {
        const QString family = line.section(QLatin1Char(':'), 0, 0).trimmed();
        if (!family.isEmpty() && !result.contains(family)) {
            result.append(family);
        }
    }
    result.sort();
    return result;
}

void SettingsController::setFont(const QString &font)
{
    if (font.isEmpty()) {
        return;
    }
    gsettings(QStringLiteral("org.gnome.desktop.interface"),
              QStringLiteral("font-name"), font);
}

QStringList SettingsController::listKeyboardShortcuts()
{
    const QString output = runCommand(
        QStringLiteral("gsettings"),
        {QStringLiteral("list-keys"),
         QStringLiteral("org.gnome.settings-daemon.plugins.media-keys")});
    return output.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
}

void SettingsController::openSystemKeyboardSettings()
{
    QProcess::startDetached(QStringLiteral("gnome-control-center"),
                           {QStringLiteral("keyboard")});
}

QString SettingsController::hostName() const
{
    return QSysInfo::machineHostName();
}

QString SettingsController::kernelVersion() const
{
    return QSysInfo::kernelVersion();
}

QString SettingsController::sessionType() const
{
    return qEnvironmentVariable("XDG_SESSION_TYPE", QStringLiteral("unknown"));
}

QString SettingsController::desktopName() const
{
    return qEnvironmentVariable("XDG_CURRENT_DESKTOP", QStringLiteral("Stellar"));
}

QString SettingsController::uptime() const
{
    const QString raw = readFileValue(QStringLiteral("/proc/uptime"));
    const double seconds = raw.section(QLatin1Char(' '), 0, 0).toDouble();
    if (seconds <= 0) {
        return {};
    }
    const int total = int(seconds);
    const int hours = total / 3600;
    const int minutes = (total % 3600) / 60;
    return tr("%1 h %2 min").arg(hours).arg(minutes);
}

void SettingsController::openTerminal()
{
    const QStringList candidates = {
        QStringLiteral("x-terminal-emulator"),
        QStringLiteral("konsole"),
        QStringLiteral("gnome-terminal"),
        QStringLiteral("alacritty"),
        QStringLiteral("foot"),
        QStringLiteral("xterm"),
    };
    for (const QString &program : candidates) {
        const QString path = QStandardPaths::findExecutable(program);
        if (!path.isEmpty()) {
            QProcess::startDetached(path, {});
            return;
        }
    }
}

void SettingsController::restartPanel()
{
    QProcess::startDetached(QStringLiteral("sh"),
                           {QStringLiteral("-c"),
                            QStringLiteral("pkill -x stellar-panel 2>/dev/null; sleep 1; setsid stellar-ui >/dev/null 2>&1 &")});
}

void SettingsController::applyAll()
{
    // Reaplica todos los ajustes persistidos al sistema.
    QIcon::setThemeName(iconTheme());
    gsettings(QStringLiteral("org.gnome.desktop.interface"),
              QStringLiteral("icon-theme"), iconTheme());
    gsettings(QStringLiteral("org.gnome.desktop.interface"),
              QStringLiteral("cursor-theme"), cursorTheme());
    gsettings(QStringLiteral("org.gnome.desktop.interface"),
              QStringLiteral("cursor-size"), QString::number(cursorSize()));
    gsettings(QStringLiteral("org.gnome.desktop.interface"),
              QStringLiteral("color-scheme"), colorScheme());

    gsettings(QStringLiteral("org.gnome.desktop.peripherals.keyboard"),
              QStringLiteral("repeat-delay"), QString::number(repeatDelay()));
    gsettings(QStringLiteral("org.gnome.desktop.peripherals.keyboard"),
              QStringLiteral("repeat-interval"), QString::number(repeatRate()));

    gsettings(QStringLiteral("org.gnome.desktop.peripherals.mouse"),
              QStringLiteral("speed"), QString::number(mouseSpeed()));
    gsettings(QStringLiteral("org.gnome.desktop.peripherals.mouse"),
              QStringLiteral("natural-scroll"),
              mouseNaturalScroll() ? QStringLiteral("true") : QStringLiteral("false"));

    gsettings(QStringLiteral("org.gnome.desktop.peripherals.touchpad"),
              QStringLiteral("send-events"),
              touchpadEnabled() ? QStringLiteral("enabled") : QStringLiteral("disabled"));
    gsettings(QStringLiteral("org.gnome.desktop.peripherals.touchpad"),
              QStringLiteral("tap-to-click"),
              touchpadTapToClick() ? QStringLiteral("true") : QStringLiteral("false"));
    gsettings(QStringLiteral("org.gnome.desktop.peripherals.touchpad"),
              QStringLiteral("natural-scroll"),
              touchpadNaturalScroll() ? QStringLiteral("true") : QStringLiteral("false"));

    applyDesktopIcons();

    // Reinicia los componentes del shell para que adopten los cambios
    // (tema de iconos, altura/posición del panel, widgets, menú...).
    QProcess::startDetached(QStringLiteral("sh"),
                           {QStringLiteral("-c"),
                            QStringLiteral("pkill -x stellar-panel 2>/dev/null; "
                                           "pkill -x stellar-menu 2>/dev/null; "
                                           "pkill -x stellar-widgets 2>/dev/null; "
                                           "pkill -x stellar-notifications 2>/dev/null; "
                                           "pkill -x stellar-background 2>/dev/null; "
                                           "sleep 1; setsid stellar-ui >/dev/null 2>&1 &")});
}