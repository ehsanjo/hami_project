#include "settings.h"

#include <QSettings>

namespace AppSettings {

QString defaultSourceUrl()
{
    return QStringLiteral(
        "https://call4.tgju.org/ajax.json?rev=oPSiDlpuNgJS9q9DvxQwKoiG9YnbtglvFyXN8rb1uMFJRaTI1LfEt32uCEwD");
}

QString sourceUrl()
{
    QSettings s;
    const QString url = s.value(QStringLiteral("source/url")).toString().trimmed();
    return url.isEmpty() ? defaultSourceUrl() : url;
}

void setSourceUrl(const QString &url)
{
    QSettings s;
    s.setValue(QStringLiteral("source/url"), url.trimmed());
}

int staleAfterHours()
{
    QSettings s;
    return qBound(1, s.value(QStringLiteral("source/staleHours"), 24).toInt(), 720);
}

void setStaleAfterHours(int hours)
{
    QSettings s;
    s.setValue(QStringLiteral("source/staleHours"), qBound(1, hours, 720));
}

bool autoRefresh()
{
    QSettings s;
    return s.value(QStringLiteral("refresh/auto"), true).toBool();
}

void setAutoRefresh(bool on)
{
    QSettings s;
    s.setValue(QStringLiteral("refresh/auto"), on);
}

int refreshIntervalSeconds()
{
    QSettings s;
    return qBound(10, s.value(QStringLiteral("refresh/seconds"), 10).toInt(), 3600);
}

void setRefreshIntervalSeconds(int seconds)
{
    QSettings s;
    s.setValue(QStringLiteral("refresh/seconds"), qBound(30, seconds, 3600));
}

} // namespace AppSettings