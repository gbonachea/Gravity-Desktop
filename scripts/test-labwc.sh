#!/usr/bin/env bash
# ==============================================================================
# Stellar Shell — prueba de humo del autostart de labwc
#
# Comprueba el rc.xml (tema por defecto, cornerRadius 0), el themerc-override
# solo con colores y la limpieza del antiguo tema Stellar.
#
# Uso:
#   scripts/test-labwc.sh [--keep]
# ==============================================================================
set -euo pipefail

KEEP=0
for arg in "$@"; do
    case "$arg" in
        --keep) KEEP=1 ;;
        -h|--help)
            sed -n '2,10p' "$0"
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
AUTOSTART="${ROOT_DIR}/autostart-labwc.sh"

TMP_HOME="$(mktemp -d /tmp/opencode/stellar-labtest-XXXXXX)"
export HOME="$TMP_HOME"
export XDG_CONFIG_HOME="$TMP_HOME/.config"

# Simula un tema Stellar generado por versiones antiguas del autostart: debe
# eliminarse para que labwc vuelva a usar su decoración por defecto.
STALE_THEME="$TMP_HOME/.themes/Stellar/openbox-3/themerc"
mkdir -p "$(dirname -- "$STALE_THEME")"
printf '# Stellar Shell theme for labwc\n' > "$STALE_THEME"

cleanup()
{
    if [[ "$KEEP" -eq 0 ]]; then
        rm -rf "$TMP_HOME"
    else
        echo "HOME temporal conservado en: $TMP_HOME"
    fi
}
trap cleanup EXIT

echo "==> Generando configuración (configure-only)…"
"$AUTOSTART" --configure-only

RC="$TMP_HOME/.config/labwc/rc.xml"
THEME_OVERRIDE="$TMP_HOME/.config/labwc/themerc-override"
AUTOSTART_FILE="$TMP_HOME/.config/labwc/autostart"

[[ -f "$RC" ]] || { echo "FAIL: falta rc.xml"; exit 1; }
[[ -f "$THEME_OVERRIDE" ]] || { echo "FAIL: falta themerc-override"; exit 1; }
[[ -f "$AUTOSTART_FILE" ]] || { echo "FAIL: falta autostart"; exit 1; }
[[ ! -e "$TMP_HOME/.themes/Stellar" ]] || { echo "FAIL: no se eliminó el tema Stellar antiguo"; exit 1; }

echo "==> Validando XML de rc.xml…"
python3 - "$RC" <<'PY'
import sys, xml.etree.ElementTree as ET
path = sys.argv[1]
try:
    tree = ET.parse(path)
except Exception as e:
    print("FAIL: rc.xml no es XML válido:", e)
    sys.exit(1)
root = tree.getroot()
assert root.tag == "labwc_config", "raíz inesperada: " + root.tag
theme = root.find("theme")
assert theme is not None, "falta <theme>"
assert theme.find("name") is None, "no debe fijarse <theme><name>"
assert theme.find("titlebar") is None, "no debe forzarse <titlebar>"
assert theme.find("maximizedDecoration") is None, "no debe forzarse maximizedDecoration"
cr = theme.find("cornerRadius")
assert cr is not None and int(cr.text) == 0, "cornerRadius debe ser 0"
print("rc.xml válido (tema por defecto + cornerRadius 0)")
PY

echo "==> Verificando themerc-override y autostart…"
grep -q "window.active.title.bg.color: #174b8f" "$THEME_OVERRIDE" || { echo "FAIL: falta el color de título activo"; exit 1; }
grep -q "window.active.label.text.color: #ffffff" "$THEME_OVERRIDE" || { echo "FAIL: falta el color del texto"; exit 1; }
grep -q "window.active.button.unpressed.image.color" "$THEME_OVERRIDE" || { echo "FAIL: faltan los colores de botones"; exit 1; }
grep -q "window.active.border.color" "$THEME_OVERRIDE" || { echo "FAIL: falta el color del borde"; exit 1; }

if grep -Eq "padding|button\.(width|height|spacing)|shadow|corner-radius" "$THEME_OVERRIDE"; then
    echo "FAIL: themerc-override no debe modificar geometría/sombras" >&2
    exit 1
fi

grep -q "stellar-ui" "$AUTOSTART_FILE" || { echo "FAIL: autostart sin stellar-ui"; exit 1; }

if command -v labwc >/dev/null 2>&1; then
    echo "==> (labwc instalado; validación de arranque omitida por permisos de seat en este entorno)"
fi

echo "PASS: configuración de labwc válida."
exit 0