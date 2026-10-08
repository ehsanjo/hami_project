#pragma once

#include <QString>

// Small wrapper around QSettings so the rest of the code never touches keys directly.
namespace AppSettings {

QString defaultSourceUrl();

QString sourceUrl();
void setSourceUrl(const QString &url);

int staleAfterHours();            // 1..720, default 24
void setStaleAfterHours(int hours);

bool autoRefresh();               // default true
void setAutoRefresh(bool on);

int refreshIntervalSeconds();     // 30..3600, default 60
void setRefreshIntervalSeconds(int seconds);

} // namespace AppSettings