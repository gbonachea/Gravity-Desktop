#!/usr/bin/env bash
# ==============================================================================
# Stellar Shell — inicio automático y configuración de Wayfire
#
# Genera ~/.config/wayfire.ini (copia de seguridad la primera vez) con:
#   - ventanas flotantes (sin tiling), decoraciones del servidor (SSD),
#   - botones minimize/maximize/close, blur, foreign-toplevel, layer-shell.
#   - arranque de stellar-ui.
#
# Uso: stellar-autostart-wayfire [--configure-only|--no-ui] [--reload] [-h]
# ==============================================================================
set -euo pipefail

SCRIPT_PATH="$(readlink -f -- "${BASH_SOURCE[0]}")"
CONFIG_HOME="${XDG_CONFIG_HOME:-$HOME/.config}"
WAYFIRE_DIR="${CONFIG_HOME}/wayfire"
WAYFIRE_CONF="${CONFIG_HOME}/wayfire.ini"
MARKER="# >>> STELLAR_SHELL_WAYFIRE >>>"

CONFIGURE_ONLY=0
RELOAD=0

usage()
{
    cat <<'EOF'
Uso: stellar-autostart-wayfire [opciones]

Opciones:
  --configure-only, --no-ui   Solo escribe la configuración; no lanza la UI.
  --reload                    (no-op en Wayfire; reinicia la sesión manualmente)
  -h, --help                  Muestra esta ayuda.

Variables de entorno:
  STELLAR_START_AUDIO=0       No intenta arrancar PipeWire/WirePlumber.
  STELLAR_START_NM_APPLET=1   Lanza nm-applet.
EOF
}

while (($# > 0)); do
    case "$1" in
        --configure-only|--no-ui) CONFIGURE_ONLY=1 ;;
        --reload) RELOAD=1 ;;
        -h|--help) usage; exit 0 ;;
        *)
            echo "stellar-autostart-wayfire: opción desconocida: $1" >&2
            usage >&2
            exit 2
            ;;
    esac
    shift
done

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
# 1. Configuración de Wayfire
# ---------------------------------------------------------------------------

mkdir -p "$WAYFIRE_DIR"

if [[ -f "$WAYFIRE_CONF" ]] && ! grep -Fq "$MARKER" "$WAYFIRE_CONF"; then
    cp -f -- "$WAYFIRE_CONF" "${WAYFIRE_CONF}.bak"
fi

cat > "$WAYFIRE_CONF" <<EOF
$MARKER
# Archivo gestionado por stellar-autostart-wayfire.

[core]
plugins = \\
  alpha \\
  animate \\
  autostart \\
  blur \\
  command \\
  decoration \\
  expo \\
  fast-switcher \\
  foreign-toplevel \\
  grid \\
  gtk-shell \\
  idle \\
  invert \\
  move \\
  oswitch \\
  place \\
  resize \\
  session-lock \\
  shortcuts-inhibit \\
  switcher \\
  vswitch \\
  wayfire-shell \\
  window-rules \\
  wm-actions \\
  wobbly \\
  wrot \\
  zoom

# Ventanas flotantes: Wayfire es stacking por defecto (sin plugin 'tile').
preferred_decoration_mode = server
close_top_view = <super> KEY_Q | <alt> KEY_F4

[autostart]
autostart_wf_shell = false
stellar-shell = stellar-ui

[decoration]
font = Segoe UI
font_color = #ffffffff
font_scale = 0.8
title_halign = left
title_valign = center
title_height = 30
border_size = 1
button_order = minimize maximize close
button_scale = 0.7
button_padding = 4
active_color = #1e4f8aee
inactive_color = #3a5f8faa
forced_views = type is "toplevel"

[blur]
method = kawase
blur_by_default = type is "toplevel" | type is "layer"
saturation = 1.0
alpha_threshold = 0.5

[input]
# configuración de teclado/ratón por defecto (wlroots)

[output]
# autodetectado
EOF

# ---------------------------------------------------------------------------
# 2. Servicios
# ---------------------------------------------------------------------------

if [[ "$CONFIGURE_ONLY" -eq 0 ]]; then
    if [[ "${STELLAR_START_AUDIO:-1}" == "1" ]]; then
        start_if_missing pipewire pipewire
        start_if_missing wireplumber wireplumber
    fi
    if [[ "${STELLAR_START_NM_APPLET:-0}" == "1" ]]; then
        start_if_missing nm-applet nm-applet --indicator
    fi
fi

# ---------------------------------------------------------------------------
# 3. Lanzar la UI
# ---------------------------------------------------------------------------

launch_ui()
{
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
    elif command -v stellar-panel >/dev/null 2>&1; then
        local bindir
        bindir="$(dirname -- "$(command -v stellar-panel)")"
        run_detached "${bindir}/stellar-background"
        run_detached "${bindir}/stellar-notifications"
        run_detached "${bindir}/stellar-menu"
        run_detached "${bindir}/stellar-widgets"
        run_detached "${bindir}/stellar-panel"
    else
        echo "stellar-autostart-wayfire: no encuentro stellar-ui ni los binarios de Stellar." >&2
        return 1
    fi
}

if [[ "$CONFIGURE_ONLY" -eq 0 ]]; then
    launch_ui
fi

exit 0
EOF
echo "written"