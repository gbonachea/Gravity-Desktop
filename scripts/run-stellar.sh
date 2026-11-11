#!/usr/bin/env bash
# Launch the complete Stellar Shell session (menu + widgets + panel).
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

# Resolve the binaries: next to the script (installed) or inside the build tree.
if [[ -x "${SCRIPT_DIR}/stellar-panel" ]]; then
    BIN_DIR="${SCRIPT_DIR}"
elif [[ -x "${SCRIPT_DIR}/../build/stellar-panel" ]]; then
    BIN_DIR="$(cd -- "${SCRIPT_DIR}/../build" && pwd)"
elif command -v stellar-panel >/dev/null 2>&1; then
    BIN_DIR="$(dirname -- "$(command -v stellar-panel)")"
else
    echo "run-stellar: no encuentro stellar-panel. Compila el proyecto primero." >&2
    exit 1
fi

if [[ -z "${WAYLAND_DISPLAY:-}" && -z "${DISPLAY:-}" ]]; then
    echo "run-stellar: no hay sesión gráfica (WAYLAND_DISPLAY/DISPLAY vacíos)." >&2
    exit 1
fi

pids=()

cleanup() {
    trap - EXIT INT TERM
    for pid in "${pids[@]}"; do
        kill "${pid}" 2>/dev/null || true
    done
    wait 2>/dev/null || true
}
trap cleanup EXIT INT TERM

# The menu starts hidden and listens on its local socket. The panel shows it
# when "Inicio" is pressed (and can spawn it if it is not running).
"${BIN_DIR}/stellar-background" &
pids+=("$!")

"${BIN_DIR}/stellar-menu" &
pids+=("$!")

# Demonio de notificaciones: publica org.freedesktop.Notifications (si está
# libre) y la interfaz org.stellar.Notifications para el panel y los widgets.
if [[ -x "${BIN_DIR}/stellar-notifications" ]]; then
    "${BIN_DIR}/stellar-notifications" &
    pids+=("$!")
fi

"${BIN_DIR}/stellar-widgets" &
pids+=("$!")

"${BIN_DIR}/stellar-panel" &
PANEL_PID=$!
pids+=("$PANEL_PID")

# Keep running while the panel lives; closing the panel ends the session.
wait "$PANEL_PID"
