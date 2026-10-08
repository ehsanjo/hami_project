#!/usr/bin/env bash
# Per-user install (no root needed). Works in two places:
#   1) from the extracted release tarball (usr/local/... next to this script)
#   2) from the project source tree (uses the binary built by build-linux.sh,
#      or a plain build in build/ or build-release/)
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
PREFIX="$HOME/.local"

BIN=""
DESKTOP=""
ICON=""

if [[ -x "$HERE/usr/local/bin/PriceTracker" ]]; then
    # Case 1: extracted tarball
    BIN="$HERE/usr/local/bin/PriceTracker"
    DESKTOP="$HERE/usr/local/share/applications/PriceTracker.desktop"
    ICON="$HERE/usr/local/share/icons/hicolor/256x256/apps/PriceTracker.png"
else
    # Case 2: project source tree (this script lives in <root>/packaging/linux)
    ROOT="$(cd "$HERE/../.." && pwd)"
    for cand in \
        "$ROOT/dist/stage/usr/local/bin/PriceTracker" \
        "$ROOT/build-release/PriceTracker" \
        "$ROOT/build/PriceTracker" \
        "$ROOT"/build/*/PriceTracker \
        "$ROOT"/build-*/PriceTracker; do
        if [[ -x "$cand" && -f "$cand" ]]; then BIN="$cand"; break; fi
    done
    DESKTOP="$ROOT/packaging/linux/PriceTracker.desktop"
    ICON="$ROOT/packaging/icon.png"
fi

if [[ -z "$BIN" || ! -f "$DESKTOP" || ! -f "$ICON" ]]; then
    echo "Could not find the built PriceTracker program."
    echo "From the project root, first run:  bash packaging/linux/build-linux.sh"
    echo "then run this script again."
    exit 1
fi

echo "Installing: $BIN"
install -Dm755 "$BIN" "$PREFIX/bin/PriceTracker"
install -Dm644 "$ICON" "$PREFIX/share/icons/hicolor/256x256/apps/PriceTracker.png"

# Desktop entry with an absolute Exec path (works even if ~/.local/bin is not in PATH)
mkdir -p "$PREFIX/share/applications"
sed "s|^Exec=.*|Exec=$PREFIX/bin/PriceTracker|" "$DESKTOP" \
    > "$PREFIX/share/applications/PriceTracker.desktop"

command -v update-desktop-database >/dev/null && \
    update-desktop-database "$PREFIX/share/applications" || true
command -v gtk-update-icon-cache >/dev/null && \
    gtk-update-icon-cache -q "$PREFIX/share/icons/hicolor" || true

echo "Installed to $PREFIX. Find 'PriceTracker' in your application menu."
echo "If it fails to start, install Qt: sudo dnf install qt6-qtbase qt6-qtcharts"
