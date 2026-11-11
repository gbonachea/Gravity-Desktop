# Stellar Shell

Shell de escritorio para Wayland con estética **glassmorphism + Windows XP remasterizado**,
escrito en C++20 y QML (Qt 6). Está compuesto por cuatro ejecutables independientes que
comparten un backend y una librería de componentes QML:

| Ejecutable | Superficie | Descripción |
| --- | --- | --- |
| `stellar-background` | Fondo absoluto (capa Background) | Dibuja el wallpaper escalado y un degradado de reserva. |
| `stellar-panel` | Barra inferior flotante | Botón **Inicio**, taskbar de ventanas activas, bandeja (Wi‑Fi, volumen, batería) y reloj. |
| `stellar-menu` | Menú Inicio flotante | Apps `.desktop`, buscador, accesos rápidos, red/sonido, diálogo **Ejecutar** y apagado/reinicio. |
| `stellar-widgets` | Tres tarjetas superiores derechas | Reloj + fecha + clima, uso de disco y estado del sistema. |

El panel y el menú se comunican por un socket local (`stellar-menu-<uid>`). El panel
arranca `stellar-menu` automáticamente si no está en ejecución.

---

## Requisitos

- **CMake** ≥ 3.21 y un compilador con **C++20**.
- **Qt 6.5+**: Core, Gui, Quick, Qml, Network y DBus.
- **LayerShellQt** (`LayerShellQt::Interface`) para usar `zwlr_layer_shell_v1`.
  Sin él, las superficies se colocan como ventanas flotantes normales.
- **libwayland-client** + `wayland-scanner` para la taskbar
  (`wlr-foreign-toplevel-management`). Sin ellos la taskbar compila vacía.
- `pactl` (PulseAudio/PipeWire) o `wpctl` (WirePlumber) para el control de volumen.
- **NetworkManager** por DBus para listar interfaces y escanear redes Wi‑Fi.
- `systemd-logind` por DBus para apagar/reiniciar/suspender.
- `xdg-open` (xdg-utils) para los accesos rápidos del menú.
- **Opcional (Hyprland)**: `hyprbars` mediante `hyprpm` para las barras de título
  con botones minimizar/maximizar/cerrar.

Nombres de paquetes orientativos:

```bash
# Arch Linux
sudo pacman -S base-devel cmake qt6-base qt6-declarative layer-shell-qt \
               wayland libpulse pipewire networkmanager systemd

# Debian / Ubuntu
sudo apt install build-essential cmake qt6-base-dev qt6-declarative-dev \
                 liblayershellqtinterface-dev libwayland-dev wayland-protocols \
                 pulseaudio-utils network-manager xdg-utils
```

---

## Compilar

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
```

Al configurar, CMake informa qué integraciones opcionales están activas:

```
StellarShell configuration:
  Qt version ............ 6.x.x
  LayerShellQt .......... TRUE
  foreign-toplevel ...... 1
```

### Ejecutar

La forma recomendada, que arranca los cuatro procesos y los cierra juntos:

```bash
./scripts/run-stellar.sh
```

También se pueden ejecutar por separado:

```bash
./build/stellar-background &
./build/stellar-menu &      # queda oculto hasta que el panel lo muestre
./build/stellar-widgets &
./build/stellar-panel
```

### Instalar

```bash
cmake --install build --prefix "$HOME/.local"
```

Instala `stellar-background`, `stellar-panel`, `stellar-menu`, `stellar-widgets`,
`stellar-settings`, `stellar-ui`, `run-stellar.sh`, `stellar-autostart`,
`stellar-autostart-wayfire`, `stellar-autostart-labwc`, `stellar-autostart-kwin`,
las sesiones `stellar-session*` en `$HOME/.local/bin`.

### Paquetes .deb

Hay un generador por compositor. Cada uno incluye únicamente su sesión Wayland,
la dependencia del compositor correspondiente y **no** incluye el tema SDDM
(que se empaqueta aparte con `scripts/build-deb.sh`):

```bash
./scripts/build-deb-hyprland.sh   # stellar-shell_0.1.0_amd64.deb
./scripts/build-deb-labwc.sh      # stellar-shell-labwc_0.1.0_amd64.deb
./scripts/build-deb-kwin.sh       # stellar-shell-kwin_0.1.0_amd64.deb
./scripts/build-deb-wayfire.sh    # stellar-shell-wayfire_0.1.0_amd64.deb
```

Los cuatro aceptan `--no-build` para reutilizar el árbol `build/` ya compilado.
El generador principal sigue disponible:

```bash
./scripts/build-deb.sh --wm hyprland [--no-build] [--skip-sddm]
```

`stellar-sddm-theme` solo se genera cuando no se pasa `--skip-sddm` (los
envoltorios por compositor lo omiten siempre).

### Hyprland

Si usas Hyprland, ejecuta una vez `stellar-autostart` (o `~/.local/bin/stellar-autostart`).
El script:

- añade `source = ~/.config/hypr/stellar.conf` a tu `hyprland.conf` **sin
  sobrescribirlo**;
- genera `stellar.conf` con ventanas flotantes (nada de mosaico), gaps, blur,
  sombras, las `layerrule` de las superficies Stellar y las barras de título de
  hyprbars;
- deja `exec-once` apuntando a sí mismo y lanza la UI (`stellar-ui`).

Las barras de título con los botones minimizar/maximizar/cerrar las dibuja el
plugin **hyprbars**, que no viene con Hyprland. Instálalo con `hyprpm`:

```bash
hyprpm update
hyprpm add https://github.com/hyprwm/hyprland-plugins
hyprpm enable hyprbars
hyprpm reload
```

Hyprland no tiene «minimizar» nativo: el botón mueve la ventana al workspace
especial `minimized`. `SUPER+Shift+M` lo muestra u oculta. Para que Qt no dibuje
una segunda barra de título sobre la de hyprbars puedes añadir
`env = QT_WAYLAND_DISABLE_WINDOWDECORATION,1` a `stellar.conf`; hazlo solo si
hyprbars está cargado, porque sin él las apps Qt se quedarían sin decoración.

> **Sobre bifurcar Hyprland:** no es la mejor opción. Hyprland no implementa
> decoraciones del lado del servidor a propósito y las apps Wayland controlan sus
> propias decoraciones (CSD); un fork tendría que mantener ese cambio contra cada
> versión y aun así no podría forzar una barra uniforme en apps GTK4/libadwaita.
> hyprbars es la vía práctica; si necesitas decoraciones nativas y minimizar real
> en todas las ventanas, KWin/Aurorae encaja mejor que un fork.

### Otros compositores

Cada compositor tiene su propio script de autostart (instalado como
`stellar-autostart-<wm>`):

- **Wayfire** (`stellar-autostart-wayfire`): genera `~/.config/wayfire.ini` con
  decoraciones del servidor (SSD), botones minimizar/maximizar/cerrar, blur
  (kawase) y `foreign-toplevel`. Wayfire es stacking por defecto.
- **labwc** (`stellar-autostart-labwc`): genera `~/.config/labwc/rc.xml`
  usando la decoración **por defecto** de labwc (sin `<theme><name>` ni
  `titlebar`/`maximizedDecoration` forzados, con `cornerRadius 0`). Solo adapta
  los colores de la barra de títulos mediante `~/.config/labwc/themerc-override`
  (título, texto, botones y borde); no toca paddings, tamaños ni sombras. labwc
  no tiene blur ni animaciones: el acrílico se degrada a translúcido.
- **KWin Wayland** (`stellar-autostart-kwin`): ajusta `~/.config/kwinrc`
  (Breeze + blur) y crea una entrada en `~/.config/autostart`. KWin 6.6.6 no
  expone `foreign-toplevel`, así que la taskbar queda vacía.

> El blur real del compositor se activa con estos scripts; sin ellos el efecto
> acrílico se aproxima con degradados y transparencias en QML.

---

## Uso

- **Inicio** abre/cierra el menú. `Esc` cierra el menú y el diálogo Ejecutar.
- El menú incluye **Configuración** (y **Red y sonido**) que abren el
  **Centro de configuración** (`stellar-settings`) y el panel nativo de red y
  sonido respectivamente.
- Los iconos de la bandeja (Wi-Fi, volumen, batería) abren ventanas emergentes con
  la lista de redes (con desplazamiento y solicitud de contraseña), control de
  volumen y estado de la batería. Clic fuera de la ventana la cierra.
- La lista de aplicaciones lee `/usr/share/applications` (y los directorios XDG de
  usuario), permite buscar y lanzar con `QProcess::startDetached()`. El botón
  **Todos los programas** amplía/reduce la lista.
- Los accesos rápidos abren documentos, la carpeta personal, el panel de control,
  ajustes, búsqueda, **Red y sonido** o el diálogo **Ejecutar**.
- **Red y sonido** abre un panel nativo con el estado de Wi‑Fi, control de volumen,
  la lista de interfaces de NetworkManager y un escáner de redes inalámbricas.
- El pie del menú permite **Bloquear**, **Suspender**, **Reiniciar** y **Apagar**
  mediante `systemd-logind`.
- En el panel, hacer clic en el reloj alterna entre formato de 12 y 24 horas.
- La bandeja del panel es interactiva: clic en Wi‑Fi escanea redes, clic en el altavoz
  silencia/activa el sonido y la rueda del ratón sube/baja el volumen. Los cambios se
  confirman con un aviso flotante temporal.
- La taskbar usa `wlr-foreign-toplevel-management`: muestra las ventanas activas,
  permite activarlas y cerrarlas.
- Las tarjetas de widgets se actualizan periódicamente y consultan el clima con
  [Open‑Meteo](https://open-meteo.com/) (sin API key).

### Fondo de pantalla

`stellar-background` crea una superficie layer-shell en la capa **Background** por cada
monitor y dibuja la imagen con recorte tipo *cover* (`PreserveAspectCrop`). La búsqueda
del wallpaper sigue este orden:

1. `STELLAR_WALLPAPER` (ruta o `file://`).
2. `~/.config/stellarshell/wallpaper` (una ruta por línea, `#` para comentarios).
3. La imagen configurada en KDE Plasma
   (`~/.config/plasma-org.kde.plasma.desktop-appletsrc`, clave `Image`).
4. El primer elemento de `usersWallpapers` en `~/.config/plasmarc`.
5. `~/Imágenes`, `~/Pictures`, `/usr/share/backgrounds` y `/usr/share/wallpapers`.

Si no encuentra ninguna imagen válida dibuja un degradado azul Stellar de reserva.

```bash
export STELLAR_WALLPAPER="/usr/share/backgrounds/RemixOS/RemixOS.jpg"
./build/stellar-background
```

### Clima

Por defecto se consulta Madrid. Se puede cambiar sin recompilar:

```bash
export STELLAR_WEATHER_CITY="Buenos Aires"
export STELLAR_WEATHER_LAT="-34.6037"
export STELLAR_WEATHER_LON="-58.3816"
./build/stellar-widgets
```

---

## Cómo funciona

- **Layer shell**: `Stellar::applyLayerSurface()` configura rol, capa, anclas,
  márgenes y zona exclusiva con LayerShellQt. Desde Qt 6.5 la integración se aplica
  por ventana, sin necesidad de `QT_WAYLAND_SHELL_INTEGRATION`. El fondo usa la capa
  **Background** anclada a los cuatro bordes; el panel se ancla abajo (márgenes de
  12 px), el menú abajo a la izquierda y los widgets arriba a la derecha.
- **Fondo de pantalla**: `WallpaperSource` resuelve la imagen y entrega a QML una URL
  `file://` ya codificada. `stellar-background` abre un `QQuickView` por `QScreen` y
  aplica a cada uno el rol `Background`, de modo que el escritorio completo queda
  cubierto en configuraciones multimonitor.
- **Lanzamiento de apps**: `Stellar::startDetached()` elimina
  `QT_WAYLAND_SHELL_INTEGRATION` del entorno de los procesos hijos. Si esa variable
  se heredara, cualquier app Qt abriría sus ventanas como superficies layer-shell y
  el compositor las mostraría maximizadas.
- **Taskbar**: los bindings de `wlr-foreign-toplevel-management` se generan con
  `wayland-scanner` en tiempo de compilación. `ToplevelModel` usa una cola de eventos
  Wayland dedicada y un timer de sondeo de 40 ms, sin depender de APIs privadas de QtWayland.
- **Estado del sistema**: `QNetworkInformation` para conectividad, `sysfs` para batería
  y `QStorageInfo` para el disco raíz.
- **SystemManager (backend nativo)**: clase compartida por el panel y el menú con
  acciones `Q_INVOKABLE`:
  - *Energía*: llamadas asíncronas a `org.freedesktop.login1` (`PowerOff`, `Reboot`,
    `Suspend`, `LockSessions`) mediante `QDBusPendingCallWatcher`, para no bloquear la
    UI mientras polkit pide autorización.
  - *Volumen*: usa `pactl` (PulseAudio/PipeWire) y cae a `wpctl` (WirePlumber) si no
    está disponible. Sondea el estado cada 4 s y aplica cambios con
    `set-sink-volume`/`set-sink-mute`.
  - *Red*: descubre interfaces con `GetDevices` de NetworkManager, lee sus propiedades
    por `org.freedesktop.DBus.Properties` y escanea redes con `RequestScan` +
    `GetAccessPoints`. Todo el recorrido es asíncrono y encadenado con watchers.
- **IPC**: `QLocalServer`/`QLocalSocket` con los comandos `toggle`, `show`, `hide` y `quit`.

---

## Estructura del proyecto

```
CMakeLists.txt
protocols/                 XML de wlr-foreign-toplevel-management
src/common/                Backend compartido (C++): SystemManager, SystemStatus,
                           WallpaperSource, AppLauncher, ToplevelModel, IPC…
src/background/            main.cpp de la superficie de fondo
src/panel/                 main.cpp del panel
src/menu/                  main.cpp + controlador de visibilidad del menú
src/widgets/               main.cpp de los widgets
src/settings/              main.cpp del centro de configuración
qml/common/                Theme.js, GlassCard, botones, iconos, glifos
qml/background/Main.qml    Wallpaper escalado + degradado de reserva
qml/panel/Main.qml         Barra inferior, taskbar y bandeja
qml/menu/                  Menú Inicio, lista de apps, accesos, energía,
                           Ejecutar y SystemPanel (red/sonido)
qml/widgets/               Tarjetas de reloj/clima, disco y estado
scripts/run-stellar.sh     Lanzador de la sesión completa
scripts/stellar-ui        Alias de run-stellar.sh para exec-once
scripts/stellar-session*  Lanzadores de sesión Wayland (Hyprland/labwc/KWin/Wayfire)
scripts/build-deb.sh      Generador principal de .deb (--wm, --skip-sddm)
scripts/build-deb-*.sh    Envoltorios de .deb por compositor
scripts/test-hyprland.sh  Prueba de humo del autostart de Hyprland
scripts/test-labwc.sh     Prueba de humo del autostart de labwc
autostart.sh               Configuración + arranque de Hyprland (stellar-autostart)
autostart-wayfire.sh       Configuración + arranque de Wayfire (stellar-autostart-wayfire)
autostart-labwc.sh         Configuración + arranque de labwc (stellar-autostart-labwc)
autostart-kwin.sh          Configuración + arranque de KWin Wayland (stellar-autostart-kwin)
```

### Variables de entorno útiles

| Variable | Efecto |
| --- | --- |
| `STELLAR_WALLPAPER` | Ruta o `file://` de la imagen de fondo. Tiene prioridad sobre la configuración de KDE. |
| `STELLAR_USE_HYRPRAPER` | `1` para usar `hyprpaper` en vez de `stellar-background`. |
| `STELLAR_START_AUDIO` | `0` para que `stellar-autostart` no intente arrancar PipeWire/WirePlumber. |
| `STELLAR_START_NM_APPLET` | `1` para lanzar `nm-applet` (requiere host de bandeja). |
| `STELLAR_WEATHER_CITY` | Ciudad mostrada en el widget del clima. |
| `STELLAR_WEATHER_LAT` / `STELLAR_WEATHER_LON` | Coordenadas para Open‑Meteo. |
| `QT_QPA_PLATFORM=offscreen` | Prueba de humo de QML sin sesión gráfica. |

> No configures `QT_WAYLAND_SHELL_INTEGRATION=layer-shell` en la sesión: LayerShellQt
> ya aplica el rol por ventana y esa variable haría que las apps lanzadas desde el
> menú se abran maximizadas.

---

## Limitaciones conocidas

- El **blur real** del compositor no se solicita: el efecto acrílico se aproxima con
  degradados, brillos y transparencias en QML.
- El menú se cierra con `Esc`, volviendo a pulsar **Inicio**, o haciendo clic fuera de
  él. Para el clic exterior se usa una superficie transparente a pantalla completa
  (`Catcher.qml`) en la capa Top, justo debajo del menú; las superficies layer-shell
  no reciben eventos fuera de su propia geometría. Un clic en cualquier zona libre del
  panel también lo cierra.
- La taskbar requiere un compositor que exponga `zwlr_foreign_toplevel_management_v1`
  (KWin, Sway, Hyprland, river, Wayfire…). En otros, aparece vacía.
- **GNOME/Mutter** no implementa `wlr-layer-shell`; el shell cae al modo de ventanas
  flotantes y el resultado no es el previsto.
- `stellar-background` crea una superficie por monitor, pero no reacciona a cambios de
  topología en caliente (conectar/desconectar pantallas): reinícialo para recolocar
  las superficies.
- El volumen depende de PulseAudio/PipeWire (`pactl`/`wpctl`); en sesiones sin servidor
  de audio el control muestra el último valor conocido y los cambios fallan con aviso.
- El escaneo Wi‑Fi usa `RequestScan` de NetworkManager; algunos adaptadores limitan la
  frecuencia de escaneo y pueden devolver error si se pulsa repetidamente.
- En **Hyprland**, las barras de título dependen del plugin `hyprbars`; sin él las
  ventanas conservan sus decoraciones propias. Muchas apps Wayland (GTK4/libadwaita)
  dibujan su propia barra incluso con hyprbars, así que puede aparecer una doble
  decoración. Es una limitación del modelo CSD de Wayland, no del shell.
- El menú muestra el nombre real del usuario (o `Usuario XP` como reserva) y usa
  `~/.face` como avatar si existe.

---

## Desarrollo

Prueba rápida de que los QML cargan sin errores, sin necesidad de sesión Wayland:

```bash
for app in stellar-background stellar-panel stellar-widgets stellar-menu; do
    QT_QPA_PLATFORM=offscreen timeout 5 "./build/$app"
done
```

Los cuatro procesos deben terminar sin avisos de QML por `stdout`. Para probar
`SystemManager` contra el sistema real (NetworkManager, logind y mezclador) conviene
ejecutarlo fuera de `offscreen` y revisar los avisos del panel de red.

Recuerda que `qml/common/Theme.js` debe empezar por `.pragma library` y que
`font.pixelSize` en QML 6 es un entero.
