#pragma once

#include <QtGlobal>

#ifdef Q_OS_ANDROID
namespace AndroidBridge {

// Starts or stops the background service (a foreground service with a notification).
void setBackgroundService(bool on);

// Entry point of the service process: no UI, just fetch prices and check alerts.
int runService(int argc, char *argv[]);

} // namespace AndroidBridge
#endif