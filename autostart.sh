#!/usr/bin/env bash
# ==============================================================================
# Stellar Shell — inicio automático y configuración de Hyprland
#
# Instalación habitual:
#   /usr/bin/stellar-autostart
#   ~/.config/hypr/scripts/autostart.sh
#
# La primera vez que se ejecuta:
#   1. Añade una línea `source = ~/.config/hypr/stellar.conf` a hyprland.conf
#      (sin sobrescribir el resto de la configuración del usuario).
#   2. Genera ~/.config/hypr/stellar.conf con las reglas flotantes, el blur,
#      las layerrules de Stellar y la configuración de hyprbars.
#   3. Lanza la UI (stellar-ui / run-stellar.sh / los cuatro binarios).
#
# En los siguientes inicios, Hyprland ejecuta este script mediante la línea
# `exec-once` que él mismo dejó en stellar.conf.
# ==============================================================================
set -euo pipefail

SCRIPT_PATH="$(readlink -f -- "${BASH_SOURCE[0]}")"
CONFIG_HOME="${XDG_CONFIG_HOME:-$HOME/.config}"
HYPR_DIR="${CONFIG_HOME}/hypr"
HYPR_CONF="${HYPR_DIR}/hyprland.conf"
STELLAR_CONF="${HYPR_DIR}/stellar.conf"
MARKER="# >>> CONFIGURACION_STELLAR_XP >>>"

CONFIGURE_ONLY=0
RELOAD=0

usage()
{
    cat <<'EOF'
Uso: stellar-autostart [opciones]

Opciones:
  --configure-only, --no-ui   Solo escribe la configuración; no lanza la UI.
  --reload                    Recarga Hyprland después de escribir la config.
  -h, --help                  Muestra esta ayuda.

Variables de entorno:
  STELLAR_USE_HYRPRAPER=1     Usa hyprpaper como fondo en vez de stellar-background.
  STELLAR_START_NM_APPLET=1   Lanza nm-applet (necesita un host de bandeja).
  STELLAR_START_AUDIO=0       No intenta arrancar PipeWire/WirePlumber.
EOF
}

while (($# > 0)); do
    case "$1" in
        --configure-only|--no-ui)
            CONFIGURE_ONLY=1
            ;;
        --reload)
            RELOAD=1
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "stellar-autostart: opción desconocida: $1" >&2
            usage >&2
            exit 2
            ;;
    esac
    shift
done

# ---------------------------------------------------------------------------
# Utilidades
# ---------------------------------------------------------------------------

run_detached()
{
    if command -v setsid >/dev/null 2>&1; then
        setsid "$@" >/dev/null 2>&1 &
    else
        nohup "$@" >/dev/null 2>&1 &
    fi
}

start_if_missing()
{
    local process="$1"
    shift
    command -v "$1" >/dev/null 2>&1 || return 0
    pgrep -x "$process" >/dev/null 2>&1 && return 0
    run_detached "$@"
}

# ---------------------------------------------------------------------------
# 1. Configuración de Hyprland (no destructiva)
# ---------------------------------------------------------------------------

mkdir -p "$HYPR_DIR"
touch "$HYPR_CONF"

# Hyprland solo lee hyprland.conf; añadimos una sola línea que incluya nuestro
# archivo gestionado. Nunca truncamos la configuración existente del usuario.
if ! grep -Fq "source = ${STELLAR_CONF}" "$HYPR_CONF"; then
    {
        printf '\n# Stellar Shell — configuración gestionada\n'
        printf 'source = %s\n' "$STELLAR_CONF"
    } >> "$HYPR_CONF"
fi

# Si en stellar.conf hay contenido ajeno (sin nuestro marcador), lo guardamos
# como copia de seguridad antes de regenerarlo.
if [[ -f "$STELLAR_CONF" ]] && ! grep -Fq "$MARKER" "$STELLAR_CONF"; then
    cp -f -- "$STELLAR_CONF" "${STELLAR_CONF}.bak"
fi

cat > "$STELLAR_CONF" <<'EOF'
# >>> CONFIGURACION_STELLAR_XP >>>
# Archivo gestionado por stellar-autostart. Los ajustes personales deben ir en
# ~/.config/hypr/hyprland.conf (o en otro archivo que este incluya).

# --- Ventanas flotantes: nada de mosaico/tiling ------------------------------
windowrule {
    name = stellar-float-all
    match:class = .*
    float = yes
}

# --- Estética general --------------------------------------------------------
# Tema de cursor por defecto (Breeze).
env = XCURSOR_THEME,breeze_cursors
env = XCURSOR_SIZE,24

general {
    gaps_in = 8
    gaps_out = 12
    border_size = 2
    col.active_border = rgba(33ccffee) rgba(00ff99ee) 45deg
    col.inactive_border = rgba(595959aa)
    resize_on_border = true
    extend_border_grab_area = 18
}

decoration {
    rounding = 16
    shadow {
        enabled = true
        range = 20
        render_power = 3
        color = rgba(00000044)
    }
    blur {
        enabled = true
        size = 8
        passes = 3
        vibrancy = 0.2
        new_optimizations = true
    }
}

# --- Blur real para las superficies layer-shell de Stellar -------------------
layerrule {
    name = stellar-blur-panel
    match:namespace = stellar-panel
    blur = true
    ignore_alpha = 0.3
}

layerrule {
    name = stellar-blur-menu
    match:namespace = stellar-menu
    blur = true
    ignore_alpha = 0.3
}

layerrule {
    name = stellar-blur-widgets
    match:namespace = stellar-widgets
    blur = true
    ignore_alpha = 0.3
}

# --- Barras de título tipo XP ------------------------------------------------
# Requiere el plugin hyprbars (hyprpm). Si no está cargado, Hyprland ignora
# estas opciones y las ventanas conservan sus decoraciones propias.
plugin {
    hyprbars {
        bar_height = 30
        bar_color = rgb(174b8f)
        col.text = rgb(ffffff)
        bar_text_font = Segoe UI
        bar_text_size = 10
        bar_text_align = left
        bar_blur = true
        bar_part_of_window = true
        bar_buttons_alignment = right
        bar_padding = 10
        bar_button_padding = 8
        icon_on_hover = false
        buttons_on_hover = false
        inactive_button_color = rgb(5a7fae)

        # Hyprbars dibuja los botones de derecha a izquierda:
        # cerrar (derecha), maximizar (centro), minimizar (izquierda).
        hyprbars-button = rgb(ef6b7a), 16, X, hyprctl dispatch killactive, rgb(ffffff)
        hyprbars-button = rgb(ffb84d), 16, □, hyprctl dispatch fullscreen 1, rgb(3a2a00)
        hyprbars-button = rgb(59aaff), 16, _, stellar-minimize, rgb(ffffff)
    }
}

# Hyprland no tiene "minimizar" nativo; las ventanas se mueven al workspace
# normal 99 (oculto para el uso habitual). El icono de la taskbar las restaura.

# Si usas hyprbars y quieres evitar la segunda barra de las apps Qt, descomenta:
# env = QT_WAYLAND_DISABLE_WINDOWDECORATION,1
EOF

# Si hyprbars está instalado como plugin del sistema, lo cargamos explícitamente.
# Si se gestiona con hyprpm, hyprpm añade su propia línea `plugin = ...`.
HYPRBARS_SO=""
for cand in /usr/lib/*/hyprland/plugins/libhyprbars.so /usr/lib/hyprland/plugins/libhyprbars.so; do
    [[ -f "$cand" ]] && { HYPRBARS_SO="$cand"; break; }
done
if [[ -n "$HYPRBARS_SO" ]] && ! grep -Fq "libhyprbars.so" "$STELLAR_CONF"; then
    awk -v so="$HYPRBARS_SO" '
        /^plugin \{/ && !done { print "plugin = " so; print ""; done = 1 }
        { print }
    ' "$STELLAR_CONF" > "${STELLAR_CONF}.tmp" && mv "${STELLAR_CONF}.tmp" "$STELLAR_CONF"
fi

# La ruta del propio script se añade aquí para no romper los heredocs y para
# que funcione tanto en /usr/bin como en el árbol de compilación.
printf '\n# Arranque de la sesión Stellar\nexec-once = %s\n' "$SCRIPT_PATH" >> "$STELLAR_CONF"

# ---------------------------------------------------------------------------
# 2. Temas globales (solo GTK; la UI Stellar tiene su propio tema QML)
# ---------------------------------------------------------------------------

if command -v gsettings >/dev/null 2>&1 \
    && gsettings list-schemas 2>/dev/null | grep -qx 'org.gnome.desktop.interface'; then
    gsettings set org.gnome.desktop.interface gtk-theme 'Adwaita' >/dev/null 2>&1 || true
    gsettings set org.gnome.desktop.interface color-scheme 'prefer-light' >/dev/null 2>&1 || true
fi

# ---------------------------------------------------------------------------
# 3. Servicios y fondo
# ---------------------------------------------------------------------------

if [[ "$CONFIGURE_ONLY" -eq 0 ]]; then
    # El fondo lo pinta stellar-background (capa Background de Wayland).
    # hyprpaper queda como opción manual para quien no use esa superficie.
    if [[ "${STELLAR_USE_HYRPRAPER:-0}" == "1" ]]; then
        start_if_missing hyprpaper hyprpaper
    fi

    # Audio: si systemd ya los gestiona, este arranque no hace nada.
    if [[ "${STELLAR_START_AUDIO:-1}" == "1" ]]; then
        start_if_missing pipewire pipewire
        start_if_missing wireplumber wireplumber
    fi

    # nm-applet necesita un host StatusNotifier; el panel Stellar ya ofrece
    # su propio panel de Red y sonido, así que no se lanza por defecto.
    if [[ "${STELLAR_START_NM_APPLET:-0}" == "1" ]]; then
        start_if_missing nm-applet nm-applet --indicator
    fi
fi

# ---------------------------------------------------------------------------
# 4. Lanzar la UI
# ---------------------------------------------------------------------------

launch_four()
{
    local bin="$1"
    for app in stellar-background stellar-notifications stellar-menu stellar-widgets stellar-panel; do
        [[ -x "${bin}/${app}" ]] && run_detached "${bin}/${app}"
    done
}

launch_ui()
{
    # Evita duplicar paneles/fondos si el script se ejecuta dos veces.
    if pgrep -x stellar-panel >/dev/null 2>&1; then
        return 0
    fi

    local dir
    dir="$(dirname -- "$SCRIPT_PATH")"

    if [[ -x "${dir}/stellar-ui" ]]; then
        run_detached "${dir}/stellar-ui"
    elif command -v stellar-ui >/dev/null 2>&1; then
        run_detached stellar-ui
    elif [[ -x "${dir}/run-stellar.sh" ]]; then
        run_detached "${dir}/run-stellar.sh"
    elif command -v run-stellar.sh >/dev/null 2>&1; then
        run_detached run-stellar.sh
    elif [[ -x "${dir}/build/stellar-panel" ]]; then
        launch_four "${dir}/build"
    elif [[ -x "${dir}/../build/stellar-panel" ]]; then
        launch_four "${dir}/../build"
    elif command -v stellar-panel >/dev/null 2>&1; then
        launch_four "$(dirname -- "$(command -v stellar-panel)")"
    else
        echo "stellar-autostart: no encuentro stellar-ui, run-stellar.sh ni los binarios de Stellar." >&2
        return 1
    fi
}

if [[ "$RELOAD" -eq 1 ]] && command -v hyprctl >/dev/null 2>&1; then
    hyprctl reload >/dev/null 2>&1 || true
fi

if [[ "$CONFIGURE_ONLY" -eq 0 ]]; then
    if command -v hyprctl >/dev/null 2>&1 \
        && ! hyprctl plugin list 2>/dev/null | grep -qi 'hyprbars'; then
        echo "stellar-autostart: aviso: hyprbars no está cargado; las ventanas no tendrán barra de título Stellar." >&2
    fi

    launch_ui
fi

exit 0
