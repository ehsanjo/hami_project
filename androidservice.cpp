#include "androidservice.h"

#ifdef Q_OS_ANDROID

#include <QCoreApplication>
#include <QJniObject>
#include <QLocale>
#include <QSettings>
#include <QTimer>
#include <QUrl>

#include "alertmanager.h"
#include "pricefetcher.h"
#include "settings.h"

namespace {

QString formatNumber(double value)
{
    static const QLocale english(QLocale::English, QLocale::UnitedStates);
    return english.toString(value, 'f', qAbs(value) < 100000 ? 2 : 0);
}

void notify(const QString &title, const QString &text, int id)
{
    QJniObject context = QNativeInterface::QAndroidApplication::context();
    QJniObject::callStaticMethod<void>(
        "org/pricetracker/Notifier", "notify",
        "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;I)V",
        context.object(),
        QJniObject::fromString(title).object<jstring>(),
        QJniObject::fromString(text).object<jstring>(),
        jint(id));
}

constexpr int kMinBackgroundSeconds = 60; // be gentle with battery and the server

} // namespace

namespace AndroidBridge {

void setBackgroundService(bool on)
{
    QJniObject context = QNativeInterface::QAndroidApplication::context();
    QJniObject::callStaticMethod<void>(
        "org/pricetracker/PriceService", on ? "start" : "stop",
        "(Landroid/content/Context;)V",
        context.object());
}

int runService(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("PriceTracker"));
    QCoreApplication::setApplicationName(QStringLiteral("PriceTracker"));

    PriceFetcher fetcher;
    AlertManager alerts;
    QTimer timer;
    timer.setSingleShot(true);
    int failures = 0;
    bool busy = false;

    auto schedule = [&]() {
        const int base = qMax(kMinBackgroundSeconds, AppSettings::refreshIntervalSeconds());
        const int factor = failures > 0 ? (1 << qMin(failures, 4)) : 1;
        timer.start(qMin(base * factor, qMax(base, 900)) * 1000);
    };

    auto poll = [&]() {
        if (busy)
            return;
        busy = true;
        QSettings().sync(); // pick up changes made by the app process
        fetcher.setUrl(QUrl(AppSettings::sourceUrl()));
        fetcher.setStaleAfterHours(AppSettings::staleAfterHours());
        alerts.reload();
        fetcher.fetch();
    };

    QObject::connect(&timer, &QTimer::timeout, &app, poll);

    QObject::connect(&fetcher, &PriceFetcher::pricesReady, &app,
                     [&](const QVector<PriceItem> &items) {
        busy = false;
        failures = 0;
        alerts.check(items);
        schedule();
    });

    QObject::connect(&fetcher, &PriceFetcher::fetchFailed, &app, [&](const QString &) {
        busy = false;
        ++failures;
        schedule();
    });

    QObject::connect(&alerts, &AlertManager::alertTriggered, &app,
                     [](const Alert &a, double price) {
        const QString direction = a.condition == Alert::Above ? QStringLiteral("at or above")
                                                              : QStringLiteral("at or below");
        const QString text = QStringLiteral("%1 is now %2\n(alert: %3 %4)")
                                 .arg(a.label, formatNumber(price), direction,
                                      formatNumber(a.threshold));
        notify(QStringLiteral("Price alert"), text, int(qHash(a.label) & 0x7fffffff));
    });

    poll();
    return app.exec();
}

} // namespace AndroidBridge

#endif // Q_OS_ANDROID