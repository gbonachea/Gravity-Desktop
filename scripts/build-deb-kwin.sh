#!/usr/bin/env bash
# ==============================================================================
# Stellar Shell — paquete .deb para la sesión KWin Wayland
#
# Genera build/deb/stellar-shell-kwin_<version>_<arch>.deb con la sesión KWin
# y sin el tema SDDM (que se empaqueta aparte). Acepta las mismas opciones que
# scripts/build-deb.sh (por ejemplo --no-build).
# ==============================================================================
set -euo pipefail
DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
exec "$DIR/build-deb.sh" --wm kwin --skip-sddm "$@"
