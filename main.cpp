#include <QCoreApplication>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTextStream>
#include <QTimeZone>
#include <QUrl>

struct WatchItem {
    const char *key;
    const char *label;
};

static const WatchItem kItems[] = {
    {"price_dollar_rl",  "Dollar (open market, rial)"},
    {"crypto-tether-irr","Tether (rial)"},
    {"geram18",          "Gold gram 18k (rial)"},
    {"geram24",          "Gold gram 24k (rial)"},
    {"mesghal",          "Gold mithqal (rial)"},
    {"sekee",            "Coin (rial)"},
    {"ons",              "Gold ounce (USD)"},
    };

static const QUrl kUrl(QStringLiteral(
    "https://call4.tgju.org/ajax.json?rev=oPSiDlpuNgJS9q9DvxQwKoiG9YnbtglvFyXN8rb1uMFJRaTI1LfEt32uCEwD"));

// "1,155,880,000" or "\t8520000" -> double
static double parseNumber(QString s)
{
    s.remove(',').remove('\t').remove(' ');
    return s.toDouble();
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QNetworkAccessManager nam;

    QNetworkRequest req(kUrl);
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  "PriceTracker/0.1 (personal student project)");
    req.setTransferTimeout(15000);

    QNetworkReply *reply = nam.get(req);

    QObject::connect(reply, &QNetworkReply::finished, &app, [reply, &app]() {
        QTextStream out(stdout);

        if (reply->error() != QNetworkReply::NoError) {
            out << "Network error: " << reply->errorString() << "\n";
            reply->deleteLater();
            app.exit(1);
            return;
        }

        const QByteArray body = reply->readAll();
        reply->deleteLater();

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            out << "JSON error: " << parseError.errorString() << "\n";
            app.exit(2);
            return;
        }

        const QJsonObject current = doc.object().value("current").toObject();
        const QLocale c = QLocale::c();
        const QTimeZone tehran("Asia/Tehran");

        for (const WatchItem &item : kItems) {
            const QString label = QString::fromUtf8(item.label).leftJustified(28);

            if (!current.contains(item.key)) {
                out << label << "  (missing in response)\n";
                continue;
            }

            const QJsonObject o = current.value(item.key).toObject();
            const double price = parseNumber(o.value("p").toString());
            const double change = parseNumber(o.value("d").toString());
            const double percent = o.value("dp").toDouble();
            const QString dir = o.value("dt").toString();
            const QString sign = dir == "high" ? "+" : (dir == "low" ? "-" : " ");

            QDateTime ts = QDateTime::fromString(o.value("ts").toString(),
                                                 "yyyy-MM-dd HH:mm:ss");
            ts.setTimeZone(tehran);
            const bool stale = !ts.isValid()
                               || ts.secsTo(QDateTime::currentDateTime()) > 24 * 3600;

            out << label
                << c.toString(price, 'f', price < 100000 ? 2 : 0).rightJustified(18)
                << "  " << sign << c.toString(change, 'f', change < 1000 ? 2 : 0)
                << " (" << c.toString(percent, 'f', 2) << "%)"
                << "  " << o.value("ts").toString()
                << (stale ? "  [STALE]" : "") << "\n";
        }

        app.quit();
    });

    return app.exec();
}