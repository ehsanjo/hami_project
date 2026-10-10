#!/usr/bin/env bash
# Removes the per-user install. Your settings and price history are kept
# unless you pass --purge.
set -euo pipefail

PREFIX="$HOME/.local"
rm -f "$PREFIX/bin/PriceTracker" \
      "$PREFIX/share/applications/PriceTracker.desktop" \
      "$PREFIX/share/icons/hicolor/256x256/apps/PriceTracker.png"

if [[ "${1:-}" == "--purge" ]]; then
    rm -rf "$HOME/.config/PriceTracker" "$HOME/.local/share/PriceTracker"
    echo "Removed program, settings and price history."
else
    echo "Removed program. Settings (~/.config/PriceTracker) and history"
    echo "(~/.local/share/PriceTracker) kept. Use --purge to delete them too."
fi
