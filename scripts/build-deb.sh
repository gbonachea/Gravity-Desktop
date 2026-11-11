#!/usr/bin/env bash
# ==============================================================================
# Stellar Shell — generador de paquete .deb
#
# Uso:
#   scripts/build-deb.sh [--no-build] [--skip-sddm] [--wm hyprland|labwc|kwin|wayfire]
#
# --no-build        Usa el árbol build/ existente sin recompilar.
# --skip-sddm       No genera el paquete independiente del tema SDDM.
# --wm <wm>         Compositor de la sesión (por defecto: hyprland).
#
# Genera (sobrescribiendo la versión actual):
#   build/deb/stellar-shell_<version>_<arch>.deb          (Hyprland)
#   build/deb/stellar-shell-labwc_<version>_<arch>.deb    (labwc)
#   build/deb/stellar-shell-kwin_<version>_<arch>.deb     (KWin Wayland)
#   build/deb/stellar-shell-wayfire_<version>_<arch>.deb  (Wayfire)
#   build/deb/stellar-sddm-theme_<version>_<arch>.deb     (tema SDDM, salvo --skip-sddm)
# ==============================================================================
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${ROOT}/build"
OUT="${BUILD}/deb"
VERSION="0.1.0"
ARCH="$(dpkg --print-architecture)"
NO_BUILD=0
SKIP_SDDM=0
WM="hyprland"

while (($# > 0)); do
    case "$1" in
        --no-build) NO_BUILD=1 ;;
        --skip-sddm) SKIP_SDDM=1 ;;
        --wm)
            shift
            WM="${1:-}"
            ;;
        -h|--help) sed -n '2,18p' "$0"; exit 0 ;;
        *) echo "Opción desconocida: $1" >&2; exit 2 ;;
    esac
    shift
done

case "$WM" in
    hyprland)
        PKGNAME="stellar-shell"
        WMDEP="hyprland, hyprland-plugin-hyprbars, procps"
        SESSION_KEEP="stellar.desktop"
        SESSION_SCRIPT="stellar-session"
        AUTOSTART_SCRIPT="stellar-autostart"
        ;;
    labwc)
        PKGNAME="stellar-shell-labwc"
        WMDEP="labwc, procps"
        SESSION_KEEP="stellar-labwc.desktop"
        SESSION_SCRIPT="stellar-session-labwc"
        AUTOSTART_SCRIPT="stellar-autostart-labwc"
        ;;
    kwin)
        PKGNAME="stellar-shell-kwin"
        WMDEP="kwin-wayland, procps"
        SESSION_KEEP="stellar-kwin.desktop"
        SESSION_SCRIPT="stellar-session-kwin"
        AUTOSTART_SCRIPT="stellar-autostart-kwin"
        ;;
    wayfire)
        PKGNAME="stellar-shell-wayfire"
        WMDEP="wayfire, procps"
        SESSION_KEEP="stellar-wayfire.desktop"
        SESSION_SCRIPT="stellar-session-wayfire"
        AUTOSTART_SCRIPT="stellar-autostart-wayfire"
        ;;
    *)
        echo "WM no válido: $WM (usa hyprland, labwc, kwin o wayfire)" >&2
        exit 2
        ;;
esac

STAGE="${BUILD}/deb/${PKGNAME}"

command -v dpkg-deb >/dev/null 2>&1 || { echo "Falta dpkg-deb" >&2; exit 1; }

if [[ "$NO_BUILD" -eq 0 ]]; then
    echo "==> Compilando…"
    cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release
    cmake --build "$BUILD" -j"$(nproc)"
fi

echo "==> Preparando árbol de instalación (${WM})…"
rm -rf "$STAGE"
mkdir -p "$STAGE"
DESTDIR="$STAGE" cmake --install "$BUILD" --prefix /usr

# El tema de SDDM vive en stellar-sddm-theme. Este paquete nunca debe
# arrastrar el tema ni su activador, aunque un build antiguo los hubiera
# dejado en el árbol de instalación.
rm -rf "$STAGE/usr/share/sddm" "$STAGE/usr/bin/stellar-sddm-theme"

# Archivos propios del empaquetado (sesión Wayland, .desktop, DEBIAN).
cp -a "$ROOT/packaging/usr" "$STAGE/"

# Solo la sesión del compositor elegido: elimina el resto de entradas Wayland,
# lanzadores de sesión y scripts de autostart de otros compositores.
for f in "$STAGE"/usr/share/wayland-sessions/*.desktop; do
    [[ -e "$f" ]] || continue
    [[ "$(basename -- "$f")" == "$SESSION_KEEP" ]] || rm -f -- "$f"
done

for f in "$STAGE"/usr/bin/stellar-session*; do
    [[ -e "$f" ]] || continue
    [[ "$(basename -- "$f")" == "$SESSION_SCRIPT" ]] || rm -f -- "$f"
done

for f in "$STAGE"/usr/bin/stellar-autostart*; do
    [[ -e "$f" ]] || continue
    [[ "$(basename -- "$f")" == "$AUTOSTART_SCRIPT" ]] || rm -f -- "$f"
done

mkdir -p "$STAGE/DEBIAN"
install -m 0755 "$ROOT/packaging/DEBIAN/postinst" "$STAGE/DEBIAN/postinst"

SIZE="$(du -sk "$STAGE" | cut -f1)"
sed -e "s/@VERSION@/$VERSION/g" \
    -e "s/@ARCH@/$ARCH/g" \
    -e "s/@SIZE@/$SIZE/g" \
    -e "s/@PKGNAME@/$PKGNAME/g" \
    -e "s/@WMDEP@/$WMDEP/g" \
    "$ROOT/packaging/DEBIAN/control.in" > "$STAGE/DEBIAN/control"

# Reduce el tamaño de los binarios.
if command -v strip >/dev/null 2>&1; then
    for bin in "$STAGE"/usr/bin/stellar-*; do
        [[ -f "$bin" ]] && strip --strip-unneeded "$bin" 2>/dev/null || true
    done
fi

# Permisos correctos.
chmod 0755 "$STAGE"/usr/bin/stellar-* "$STAGE"/usr/bin/run-stellar.sh \
    "$STAGE"/usr/bin/stellar-ui 2>/dev/null || true
find "$STAGE" -type d -exec chmod 0755 {} +
chmod 0644 "$STAGE"/usr/share/applications/*.desktop \
    "$STAGE"/usr/share/wayland-sessions/*.desktop 2>/dev/null || true

echo "==> Generando .deb…"
mkdir -p "$OUT"
DEB="${OUT}/${PKGNAME}_${VERSION}_${ARCH}.deb"
rm -f "$DEB"
dpkg-deb --build --root-owner-group "$STAGE" "$DEB" >/dev/null

if dpkg-deb --contents "$DEB" | grep -q "usr/share/sddm/themes"; then
    echo "ERROR: el paquete ${PKGNAME} contiene el tema SDDM; debe ir solo en stellar-sddm-theme." >&2
    exit 1
fi

echo
echo "Paquete generado: $DEB"
dpkg-deb --info "$DEB" | sed -n '1,20p'
echo
echo "Sesión incluida:"
dpkg-deb --contents "$DEB" | grep "wayland-sessions"

# ---------------------------------------------------------------------------
# Paquete independiente del tema SDDM
# ---------------------------------------------------------------------------
SDDM_PKGNAME="stellar-sddm-theme"
SDDM_STAGE="${BUILD}/deb/${SDDM_PKGNAME}"

build_sddm_theme() {
    echo
    echo "==> Preparando paquete del tema SDDM…"
    rm -rf "$SDDM_STAGE"
    mkdir -p "$SDDM_STAGE"

    # Contenido del tema y el activador manual.
    cp -a "$ROOT/packaging/sddm-theme/usr" "$SDDM_STAGE/"

    mkdir -p "$SDDM_STAGE/DEBIAN"
    install -m 0755 "$ROOT/packaging/sddm-theme/DEBIAN/postinst" \
        "$SDDM_STAGE/DEBIAN/postinst"
    install -m 0755 "$ROOT/packaging/sddm-theme/DEBIAN/postrm" \
        "$SDDM_STAGE/DEBIAN/postrm"

    local sddm_size
    sddm_size="$(du -sk "$SDDM_STAGE" | cut -f1)"
    sed -e "s/@VERSION@/$VERSION/g" \
        -e "s/@ARCH@/$ARCH/g" \
        -e "s/@SIZE@/$sddm_size/g" \
        -e "s/@PKGNAME@/$SDDM_PKGNAME/g" \
        "$ROOT/packaging/sddm-theme/DEBIAN/control.in" > "$SDDM_STAGE/DEBIAN/control"

    chmod 0755 "$SDDM_STAGE/usr/bin/stellar-sddm-theme"
    find "$SDDM_STAGE" -type d -exec chmod 0755 {} +
    chmod 0644 "$SDDM_STAGE"/usr/share/sddm/themes/stellar/*

    mkdir -p "$OUT"
    local sddm_deb="${OUT}/${SDDM_PKGNAME}_${VERSION}_${ARCH}.deb"
    rm -f "$sddm_deb"
    dpkg-deb --build --root-owner-group "$SDDM_STAGE" "$sddm_deb" >/dev/null

    echo "Paquete generado: $sddm_deb"
    dpkg-deb --info "$sddm_deb" | sed -n '1,20p'
    echo
    echo "Tema incluido:"
    dpkg-deb --contents "$sddm_deb" | grep "sddm/themes"
}

if [[ "$SKIP_SDDM" -eq 0 ]]; then
    build_sddm_theme
fi
