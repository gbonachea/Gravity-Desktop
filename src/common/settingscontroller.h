#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QVariantList>

/*!
 * Persistencia de los ajustes propios de Stellar Shell (panel, menú, widgets,
 * fondo de pantalla, tema...). Los ajustes del sistema (teclado, ratón, etc.)
 * se delegan a las herramientas nativas (gsettings, xrandr, timedatectl).
 */
class SettingsController : public QObject
{
    Q_OBJECT

    // Panel
    Q_PROPERTY(int panelHeight READ panelHeight WRITE setPanelHeight NOTIFY panelHeightChanged)
    Q_PROPERTY(QString panelPosition READ panelPosition WRITE setPanelPosition NOTIFY panelPositionChanged)
    Q_PROPERTY(bool panelAutoHide READ panelAutoHide WRITE setPanelAutoHide NOTIFY panelAutoHideChanged)
    Q_PROPERTY(int panelOpacity READ panelOpacity WRITE setPanelOpacity NOTIFY panelOpacityChanged)

    // Menú
    Q_PROPERTY(bool menuShowSearch READ menuShowSearch WRITE setMenuShowSearch NOTIFY menuShowSearchChanged)
    Q_PROPERTY(bool menuShowRecent READ menuShowRecent WRITE setMenuShowRecent NOTIFY menuShowRecentChanged)
    Q_PROPERTY(QStringList menuFavorites READ menuFavorites WRITE setMenuFavorites NOTIFY menuFavoritesChanged)
    Q_PROPERTY(QVariantList appChoices READ appChoices NOTIFY menuFavoritesChanged)

    // Widgets
    Q_PROPERTY(bool widgetClock READ widgetClock WRITE setWidgetClock NOTIFY widgetsChanged)
    Q_PROPERTY(bool widgetStorage READ widgetStorage WRITE setWidgetStorage NOTIFY widgetsChanged)
    Q_PROPERTY(bool widgetStatus READ widgetStatus WRITE setWidgetStatus NOTIFY widgetsChanged)
    Q_PROPERTY(QStringList widgetOrder READ widgetOrder WRITE setWidgetOrder NOTIFY widgetsChanged)
    Q_PROPERTY(QStringList visibleWidgets READ visibleWidgets NOTIFY widgetsChanged)
    Q_PROPERTY(bool weatherAutoLocate READ weatherAutoLocate WRITE setWeatherAutoLocate
                   NOTIFY weatherLocationChanged)
    Q_PROPERTY(QString weatherCity READ weatherCity WRITE setWeatherCity
                   NOTIFY weatherLocationChanged)

    // Apariencia
    Q_PROPERTY(QString wallpaper READ wallpaper WRITE setWallpaper NOTIFY wallpaperChanged)
    Q_PROPERTY(QString iconTheme READ iconTheme WRITE setIconTheme NOTIFY iconThemeChanged)
    Q_PROPERTY(QString cursorTheme READ cursorTheme WRITE setCursorTheme NOTIFY cursorThemeChanged)
    Q_PROPERTY(int cursorSize READ cursorSize WRITE setCursorSize NOTIFY cursorSizeChanged)
    Q_PROPERTY(QString colorScheme READ colorScheme WRITE setColorScheme NOTIFY colorSchemeChanged)
    Q_PROPERTY(bool desktopShowComputer READ desktopShowComputer WRITE setDesktopShowComputer NOTIFY desktopIconsChanged)
    Q_PROPERTY(bool desktopShowUser READ desktopShowUser WRITE setDesktopShowUser NOTIFY desktopIconsChanged)
    Q_PROPERTY(bool desktopShowTrash READ desktopShowTrash WRITE setDesktopShowTrash NOTIFY desktopIconsChanged)
    Q_PROPERTY(bool notificationsMuted READ notificationsMuted WRITE setNotificationsMuted
                   NOTIFY notificationsSettingsChanged)
    Q_PROPERTY(bool notificationsTrayIcon READ notificationsTrayIcon WRITE setNotificationsTrayIcon
                   NOTIFY notificationsSettingsChanged)
    Q_PROPERTY(bool notificationsPopup READ notificationsPopup WRITE setNotificationsPopup
                   NOTIFY notificationsSettingsChanged)

    // Pantalla
    Q_PROPERTY(int brightness READ brightness NOTIFY brightnessChanged)

    // Teclado
    Q_PROPERTY(int repeatDelay READ repeatDelay WRITE setRepeatDelay NOTIFY repeatDelayChanged)
    Q_PROPERTY(int repeatRate READ repeatRate WRITE setRepeatRate NOTIFY repeatRateChanged)

    // Ratón / touchpad
    Q_PROPERTY(double mouseSpeed READ mouseSpeed WRITE setMouseSpeed NOTIFY mouseSpeedChanged)
    Q_PROPERTY(bool mouseNaturalScroll READ mouseNaturalScroll WRITE setMouseNaturalScroll NOTIFY mouseNaturalScrollChanged)
    Q_PROPERTY(bool touchpadEnabled READ touchpadEnabled WRITE setTouchpadEnabled NOTIFY touchpadEnabledChanged)
    Q_PROPERTY(bool touchpadTapToClick READ touchpadTapToClick WRITE setTouchpadTapToClick NOTIFY touchpadTapToClickChanged)
    Q_PROPERTY(bool touchpadNaturalScroll READ touchpadNaturalScroll WRITE setTouchpadNaturalScroll NOTIFY touchpadNaturalScrollChanged)

public:
    explicit SettingsController(QObject *parent = nullptr);

    // Recarga los ajustes escritos por otro proceso (Centro de configuración)
    // y devuelve la ruta del archivo de configuración para poder vigilarlo.
    void reloadSettings();
    QString settingsFilePath() const;

    // Panel
    int panelHeight() const;
    void setPanelHeight(int value);
    QString panelPosition() const;
    void setPanelPosition(const QString &value);
    bool panelAutoHide() const;
    void setPanelAutoHide(bool value);
    int panelOpacity() const;
    void setPanelOpacity(int value);

    // Menú
    bool menuShowSearch() const;
    void setMenuShowSearch(bool value);
    bool menuShowRecent() const;
    void setMenuShowRecent(bool value);
    QStringList menuFavorites() const;
    void setMenuFavorites(const QStringList &value);
    QVariantList appChoices() const;
    Q_INVOKABLE bool isFavorite(const QString &file) const;
    Q_INVOKABLE void toggleFavorite(const QString &file);
    Q_INVOKABLE void moveFavorite(const QString &file, int delta);
    Q_INVOKABLE QString appName(const QString &file) const;
    Q_INVOKABLE QString appIcon(const QString &file) const;

    // Widgets
    bool widgetClock() const;
    void setWidgetClock(bool value);
    bool widgetStorage() const;
    void setWidgetStorage(bool value);
    bool widgetStatus() const;
    void setWidgetStatus(bool value);
    QStringList widgetOrder() const;
    void setWidgetOrder(const QStringList &value);
    QStringList visibleWidgets() const;
    Q_INVOKABLE void moveWidget(const QString &name, int delta);
    Q_INVOKABLE QString widgetLabel(const QString &name) const;
    bool weatherAutoLocate() const;
    void setWeatherAutoLocate(bool value);
    QString weatherCity() const;
    void setWeatherCity(const QString &value);

    // Apariencia
    QString wallpaper() const;
    void setWallpaper(const QString &value);
    QString iconTheme() const;
    void setIconTheme(const QString &value);
    QString cursorTheme() const;
    void setCursorTheme(const QString &value);
    int cursorSize() const;
    void setCursorSize(int value);
    QString colorScheme() const;
    void setColorScheme(const QString &value);
    bool desktopShowComputer() const;
    void setDesktopShowComputer(bool value);
    bool desktopShowUser() const;
    void setDesktopShowUser(bool value);
    bool desktopShowTrash() const;
    void setDesktopShowTrash(bool value);
    Q_INVOKABLE bool pinToDesktop(const QString &desktopFile);
    bool notificationsMuted() const;
    void setNotificationsMuted(bool value);
    bool notificationsTrayIcon() const;
    void setNotificationsTrayIcon(bool value);
    bool notificationsPopup() const;
    void setNotificationsPopup(bool value);

    // Pantalla
    int brightness() const;
    Q_INVOKABLE void setBrightness(int percent);

    // Teclado
    int repeatDelay() const;
    void setRepeatDelay(int value);
    int repeatRate() const;
    void setRepeatRate(int value);

    // Ratón / touchpad
    double mouseSpeed() const;
    Q_INVOKABLE void setMouseSpeed(double value);
    bool mouseNaturalScroll() const;
    void setMouseNaturalScroll(bool value);
    bool touchpadEnabled() const;
    Q_INVOKABLE void setTouchpadEnabled(bool value);
    bool touchpadTapToClick() const;
    void setTouchpadTapToClick(bool value);
    bool touchpadNaturalScroll() const;
    void setTouchpadNaturalScroll(bool value);

    Q_INVOKABLE QString runCommand(const QString &program, const QStringList &args);
    Q_INVOKABLE QStringList listIconThemes();
    Q_INVOKABLE QStringList listCursorThemes();
    Q_INVOKABLE QStringList listDisplayOutputs();
    Q_INVOKABLE bool setDisplayMode(const QString &output, const QString &mode);
    Q_INVOKABLE QString currentDateTime();
    Q_INVOKABLE void setAutoTime(bool enabled);
    Q_INVOKABLE void setTimeZone(const QString &zone);
    Q_INVOKABLE void setKeyboardLayout(const QString &layout);
    Q_INVOKABLE QStringList powerProfiles();
    Q_INVOKABLE void setPowerProfile(const QString &profile);
    Q_INVOKABLE QString currentPowerProfile();
    Q_INVOKABLE QStringList listFonts();
    Q_INVOKABLE void setFont(const QString &font);
    Q_INVOKABLE QStringList listKeyboardShortcuts();
    Q_INVOKABLE void openSystemKeyboardSettings();
    Q_INVOKABLE QString hostName() const;
    Q_INVOKABLE QString kernelVersion() const;
    Q_INVOKABLE QString sessionType() const;
    Q_INVOKABLE QString desktopName() const;
    Q_INVOKABLE QString uptime() const;
    Q_INVOKABLE void openTerminal();
    Q_INVOKABLE void restartPanel();
    Q_INVOKABLE void applyAll();

signals:
    void panelHeightChanged();
    void panelPositionChanged();
    void panelAutoHideChanged();
    void panelOpacityChanged();
    void menuShowSearchChanged();
    void menuShowRecentChanged();
    void menuFavoritesChanged();
    void widgetsChanged();
    void weatherLocationChanged();
    void wallpaperChanged();
    void iconThemeChanged();
    void cursorThemeChanged();
    void cursorSizeChanged();
    void colorSchemeChanged();
    void desktopIconsChanged();
    void notificationsSettingsChanged();
    void brightnessChanged();
    void repeatDelayChanged();
    void repeatRateChanged();
    void mouseSpeedChanged();
    void mouseNaturalScrollChanged();
    void touchpadEnabledChanged();
    void touchpadTapToClickChanged();
    void touchpadNaturalScrollChanged();

private:
    void gsettings(const QString &schema, const QString &key, const QString &value);
    static QStringList defaultMenuFavorites();
    QString desktopDir() const;
    bool writeDesktopEntry(const QString &fileName, const QString &name,
                           const QString &exec, const QString &icon);
    bool removeDesktopEntry(const QString &fileName);
    void applyDesktopIcons();

    QSettings m_settings;
};