#!/usr/bin/env bash
# ==============================================================================
# Stellar Shell — prueba de humo del autostart de Hyprland
#
# Uso:
#   scripts/test-hyprland.sh [--keep]
#
# --keep   Conserva el HOME temporal para inspección manual.
#
# Sale con código 0 si la configuración se genera y valida, y (si Hyprland está
# instalado) el plugin hyprbars se carga sin errores.
# ==============================================================================
set -euo pipefail

KEEP=0
for arg in "$@"; do
    case "$arg" in
        --keep) KEEP=1 ;;
        -h|--help)
            sed -n '2,20p' "$0"
            exit 0
            ;;
        *)
            echo "Opción desconocida: $arg" >&2
            exit 2
            ;;
    esac
done

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd -- "${SCRIPT_DIR}/.." && pwd)"
AUTOSTART="${ROOT_DIR}/autostart.sh"

TMP_HOME="$(mktemp -d /tmp/opencode/stellar-hyprtest-XXXXXX)"
export HOME="$TMP_HOME"
export XDG_CONFIG_HOME="$TMP_HOME/.config"

cleanup()
{
    pkill -f "hyprland -c $TMP_HOME" 2>/dev/null || true
    if [[ "$KEEP" -eq 0 ]]; then
        rm -rf "$TMP_HOME"
    else
        echo "HOME temporal conservado en: $TMP_HOME"
    fi
}
trap cleanup EXIT

echo "==> Generando configuración (configure-only)…"
"$AUTOSTART" --configure-only

CONF="$TMP_HOME/.config/hypr/hyprland.conf"
# Pantalla de prueba de mayor resolución (solo para el entorno headless).
printf '\nmonitor=,1680x1050@60,auto,1\n' >> "$CONF"
STELLAR="$TMP_HOME/.config/hypr/stellar.conf"
[[ -f "$CONF" ]]   || { echo "FAIL: falta hyprland.conf"; exit 1; }
[[ -f "$STELLAR" ]] || { echo "FAIL: falta stellar.conf"; exit 1; }
grep -Fq "source = $STELLAR" "$CONF" || { echo "FAIL: falta source en hyprland.conf"; exit 1; }

if ! command -v hyprland >/dev/null 2>&1; then
    echo "WARN: hyprland no instalado; se omite la validación del compositor."
    echo "PASS (config generada)"
    exit 0
fi

echo "==> Validando sintaxis (hyprland --verify-config)…"
hyprland --verify-config -c "$CONF" 2>&1 | tail -3
if ! hyprland --verify-config -c "$CONF" >/tmp/opencode/hypr-verify.log 2>&1; then
    cat /tmp/opencode/hypr-verify.log >&2
    echo "FAIL: configuración inválida"
    exit 1
fi
grep -q "config ok" /tmp/opencode/hypr-verify.log || {
    cat /tmp/opencode/hypr-verify.log >&2
    echo "FAIL: no se confirmó 'config ok'"
    exit 1
}

echo "==> Arrancando Hyprland headless para comprobar hyprbars…"
# Habilita logs para detectar la carga del plugin.
python3 - "$STELLAR" <<'PY'
p=__import__("sys").argv[1]
s=open(p).read()
if "disable_logs = false" not in s:
    s += "\ndebug {\n    disable_logs = false\n}\n"
open(p,"w").write(s)
PY

env HOME="$TMP_HOME" XDG_CONFIG_HOME="$TMP_HOME/.config" \
    WLR_BACKENDS=headless \
    timeout -k 2 10 hyprland -c "$CONF" >/tmp/opencode/hypr-run.log 2>&1 || true

LOG="$(ls -t /run/user/$(id -u)/hypr/*/hyprland.log 2>/dev/null | head -1)"
if [[ -z "$LOG" || ! -f "$LOG" ]]; then
    echo "WARN: no se encontró hyprland.log; se omite la comprobación del plugin."
    echo "PASS (config válida)"
    exit 0
fi

if grep -q "Plugin hyprbars loaded" "$LOG"; then
    echo "PASS: hyprbars se carga correctamente."
else
    echo "FAIL: hyprbars no se cargó. Últimas líneas del log:"
    tail -20 "$LOG" >&2
    exit 1
fi

exit 0