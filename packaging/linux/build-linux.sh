#!/usr/bin/env bash
# Build, test and package PriceTracker on Fedora (or any Linux with Qt6).
# Usage: run from the project root:   bash packaging/linux/build-linux.sh
# Output: dist/PriceTracker-<version>-linux-x86_64.tar.gz
set -euo pipefail

VERSION="1.0.0"
ARCH="$(uname -m)"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD="$ROOT/build-release"
STAGE="$ROOT/dist/stage"
OUT="$ROOT/dist/PriceTracker-$VERSION-linux-$ARCH.tar.gz"

echo "== 1/4 Configure (Release)"
cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local

echo "== 2/4 Build"
cmake --build "$BUILD" -j"$(nproc)"

echo "== 3/4 Run tests"
ctest --test-dir "$BUILD" --output-on-failure

echo "== 4/4 Stage and pack"
rm -rf "$STAGE"
DESTDIR="$STAGE" cmake --install "$BUILD"

# Add the user-level install script to the package
install -m 755 "$ROOT/packaging/linux/install.sh" "$STAGE/install.sh"
install -m 755 "$ROOT/packaging/linux/uninstall.sh" "$STAGE/uninstall.sh"

mkdir -p "$ROOT/dist"
tar -C "$STAGE" -czf "$OUT" .
echo
echo "Done: $OUT"
echo "Needs on the target machine: Qt6 base, Qt6 Charts, Qt6 SQL (sqlite). On Fedora:"
echo "  sudo dnf install qt6-qtbase qt6-qtcharts qt6-qtbase-common"
