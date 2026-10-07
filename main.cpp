#include <QCoreApplication>
#include <QLocale>
#include <QTextStream>

#include "pricefetcher.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    PriceFetcher fetcher;

    QObject::connect(&fetcher, &PriceFetcher::pricesReady, &app,
                     [&app](const QVector<PriceItem> &items) {
                         QTextStream out(stdout);
                         const QLocale c = QLocale::c();

                         for (const PriceItem &it : items) {
                             const QString label = it.label.leftJustified(28);
                             if (!it.found) {
                                 out << label << "  (missing in response)\n";
                                 continue;
                             }
                             out << label
                                 << c.toString(it.price, 'f', it.price < 100000 ? 2 : 0).rightJustified(18)
                                 << "  " << (it.change >= 0 ? "+" : "")
                                 << c.toString(it.change, 'f', qAbs(it.change) < 1000 ? 2 : 0)
                                 << " (" << (it.percent >= 0 ? "+" : "")
                                 << c.toString(it.percent, 'f', 2) << "%)"
                                 << "  " << it.updated.toString("yyyy-MM-dd HH:mm:ss")
                                 << (it.stale ? "  [STALE]" : "") << "\n";
                         }
                         app.quit();
                     });

    QObject::connect(&fetcher, &PriceFetcher::fetchFailed, &app,
                     [&app](const QString &message) {
                         QTextStream(stderr) << "Fetch failed: " << message << "\n";
                         app.exit(1);
                     });

    fetcher.fetch();
    return app.exec();
}