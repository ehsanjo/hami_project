# PriceTracker packaging (M9)

## Files

    packaging/
      icon.png, icon.ico
      CMakeLists-install-snippet.txt   <- paste at the end of your CMakeLists.txt
      linux/   PriceTracker.desktop, build-linux.sh, install.sh, uninstall.sh
      windows/ PriceTracker.iss, build-windows.bat

## One-time step: CMake install rules

Open the top-level `CMakeLists.txt`, paste the content of
`packaging/CMakeLists-install-snippet.txt` at the very end, save, and
re-run CMake. It adds `install()` rules (exe, .desktop file, icon) and makes
the Windows build a GUI app with no console window.

## Linux (Fedora)

    bash packaging/linux/build-linux.sh

It builds in Release mode, runs the tests, and creates
`dist/PriceTracker-1.0.0-linux-x86_64.tar.gz`.

To install it on a machine: extract the tarball, then run `./install.sh`
(installs into `~/.local`, adds a menu entry, no root needed).
Remove with `./uninstall.sh` (add `--purge` to also delete settings and history).

The target machine needs: `sudo dnf install qt6-qtbase qt6-qtcharts`

## Windows

Needs: Qt 6 for Windows (MinGW or MSVC kit), Ninja (comes with Qt Tools),
and Inno Setup 6 (https://jrsoftware.org/isinfo.php).

1. Open the "Qt 6.x (MinGW)" command prompt from the Start menu.
2. Set `QT_DIR` to your kit folder, for example
   `set QT_DIR=C:\Qt\6.8.0\mingw_64`
3. From the project root run `packaging\windows\build-windows.bat`

Result: `dist\windows\PriceTracker-1.0.0-setup.exe`, a normal installer
(per-user by default, optional desktop shortcut and start-with-Windows).

## Checklist before release

- All tests pass (`ctest` runs inside both build scripts).
- Fresh install: start the app, press Refresh, prices appear.
- Alerts and tray icon work; History tab shows rows after a few refreshes.
- Uninstall leaves no leftovers except your settings and `prices.sqlite`.
