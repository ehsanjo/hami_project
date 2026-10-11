# Gold & Dollar Tracker

A small Qt 6 / C++ app that shows live **gold and dollar prices** from [tgju.org](https://www.tgju.org), keeps a price history, draws charts, does simple analysis and sends **price alerts**. It runs on **Linux, Windows and Android** from one code base.

> Prices come from an unofficial public JSON endpoint of tgju.org. The format or the address can change at any time. This app is for personal information only, not financial advice.

## Features

- **Live prices**: dollar (open market), Tether, gold gram 18k / 24k, mithqal, coin, global gold ounce (USD) and a custom "Comparison" row. Green and red colors for up and down, gray for stale data.
- **Auto refresh**: every 10 s or more. After repeated failures it slows down (2x, 4x, 8x, at most 15 minutes) and shows how many failures happened.
- **Chart**: line chart of any item for the last hour, 24 hours, 7 days, 30 days or all data (Qt Charts).
- **Analysis**: change %, high, low, average, and gold/dollar ratios (USD per gram, local vs global price).
- **Alerts**: "price at or above / at or below X". An alert fires once and re-arms when the condition becomes false. Stale data never triggers an alert.
- **History**: every price update is saved to a local SQLite database and shown in a table.
- **Settings**: source URL, stale limit (hours), auto refresh and interval.
- **Desktop**: tray icon (when the desktop has one), close-to-tray, notifications.
- **Android**: touch layout (bottom tabs, kinetic scrolling, large buttons), Android notifications, Back button goes to the first tab, optional **background alerts**.

## Tabs

| Tab | What it does |
| --- | --- |
| Live | Current prices, refresh button, auto refresh |
| Chart | Price chart with item and range selectors |
| Analysis | Statistics and gold/dollar ratios |
| Alerts | Add and remove price alerts |
| History | Saved prices, newest first |
| Settings | Source URL, stale limit |

## Data source

The app reads `https://call4.tgju.org/ajax.json` (the same data the website uses). Like the website, every request gets a new random `rev=` value and the app rotates between the `call2`, `call3` and `call4` servers, so it always gets fresh data.

Prices are in rial, except the global ounce, which is in USD. When the Iranian market is closed, local items show the last daily record, so they can be marked **stale**. Raise the stale limit in Settings (for example 72 hours) if you want them shown as normal.

## Build

Requirements: Qt 6.5 or newer with the modules **Core, Network, Widgets, Sql, Charts** (and **Test** on desktop), CMake 3.21 or newer, a C++17 compiler.

### Desktop (Linux / Windows)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/PriceTracker
```

Run the unit tests:

```bash
ctest --test-dir build --output-on-failure
```

### Android

Android builds run on GitHub Actions (`.github/workflows/android.yml`): Qt 6.8.3, NDK 26.1.10909125, target `android_arm64_v8a`. The APK is signed and published as a GitHub Release (`build-N`).

Add these repository secrets before the first run:

| Secret | Meaning |
| --- | --- |
| `KEYSTORE_BASE64` | your keystore file, base64 encoded |
| `KEYSTORE_PASSWORD` | keystore password |
| `KEY_ALIAS` | key alias inside the keystore |

Keep the same keystore for every release, otherwise Android will refuse to update the installed app.

### Packaging

- **Windows installer**: `.github/workflows/windows-installer.yml` (manual run, or push a tag like `v1.3.0`). Uses Inno Setup. The setup `.exe` is attached to the release.
- **Linux**: install and build scripts in `packaging/linux`. See `PACKAGING.md`.

## Android notes

- **Install**: download the APK from the Releases page (do not use zipped artifacts) and allow installing from your browser or file manager.
- **Notifications**: on Android 13 or newer the app asks for notification permission at the first start. Allow it, or alerts will be silent.
- **Alerts while the app is open**: always work.
- **Background alerts**: tick *Background alerts* on the Live tab. A small ongoing notification ("Watching prices for your alerts") appears while it runs, and the check runs at most once per minute. Untick it to stop the service.
- **Battery**: some phones (Xiaomi, Huawei, Samsung) stop background apps anyway. Set the app to *Unrestricted* in the phone's battery settings, and enable autostart if your phone has that option.
- The service does not start by itself after a reboot. Open the app once.

## Project structure

```
main.cpp, mainwindow.*         window, tabs, tray, Android touch tweaks
chartwidget.*                  Qt Charts price chart
analysiswidget.*               statistics and ratios
alertswidget.*                 alert list and editor
settingswidget.*               settings page
androidservice.*               Android background service (Android only)
pricefetcher.*                 network request and JSON parsing (PriceCore)
pricedatabase.*                SQLite history (PriceCore)
analysis.*                     statistics math (PriceCore)
alertmanager.*                 alert rules and storage (PriceCore)
settings.*                     QSettings wrapper (PriceCore)
android/                       manifest, icons, Java helpers (notifications, service)
packaging/                     Windows (Inno Setup) and Linux packaging
tests/tst_core.cpp             unit tests (QtTest)
```

`PriceCore` is a static library with no GUI code. The app, the tests and the Android background service all share it.

## Where data is stored

| Platform | Settings | Database |
| --- | --- | --- |
| Linux | `~/.config/PriceTracker` | `~/.local/share/PriceTracker/prices.sqlite` |
| Windows | registry, `PriceTracker` | `%AppData%/PriceTracker/prices.sqlite` |
| Android | app private storage | app private storage (removed when the app is uninstalled) |

## Troubleshooting

- **"None of the expected prices were found"**: the site format or the URL changed. Check the source URL in Settings.
- **Everything is gray / "stale"**: the market is closed or the stale limit is too low. Raise it in Settings.
- **TLS / HTTPS error on Android**: the build must include KDAB `android_openssl` (the workflow clones it).
- **No alert notifications on Android**: check the notification permission and the battery setting above.

## License

Add your license here (for example MIT).
