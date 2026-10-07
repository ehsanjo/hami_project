#include "pricefetcher.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimeZone>

namespace {

struct WatchItem {
    const char *key;
    const char *label;
};

const WatchItem kWatchList[] = {
    {"price_dollar_rl",   "Dollar (open market, rial)"},
    {"crypto-tether-irr", "Tether (rial)"},
    {"geram18",           "Gold gram 18k (rial)"},
    {"geram24",           "Gold gram 24k (rial)"},
    {"mesghal",           "Gold mithqal (rial)"},
    {"sekee",             "Coin (rial)"},
    {"ons",               "Gold ounce (USD)"},
    };

// "1,155,880,000" or "\t8520000" -> double
double parseNumber(QString s)
{
    s.remove(',').remove('\t').remove(' ');
    return s.toDouble();
}

} // namespace

PriceFetcher::PriceFetcher(QObject *parent)
    : QObject(parent)
    , m_url(QStringLiteral(
          "https://call4.tgju.org/ajax.json?rev=oPSiDlpuNgJS9q9DvxQwKoiG9YnbtglvFyXN8rb1uMFJRaTI1LfEt32uCEwD"))
{
}

void PriceFetcher::fetch()
{
    QNetworkRequest req(m_url);
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  "PriceTracker/0.2 (personal student project)");
    req.setTransferTimeout(15000);

    QNetworkReply *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit fetchFailed(reply->errorString());
            return;
        }

        QString error;
        const QVector<PriceItem> items = parseResponse(reply->readAll(), &error);
        if (!error.isEmpty()) {
            emit fetchFailed(error);
            return;
        }
        emit pricesReady(items);
    });
}

QVector<PriceItem> PriceFetcher::parseResponse(const QByteArray &body, QString *error)
{
    QVector<PriceItem> result;

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (error)
            *error = QStringLiteral("JSON error: ") + parseError.errorString();
        return result;
    }

    const QJsonValue currentValue = doc.object().value(QStringLiteral("current"));
    if (!currentValue.isObject()) {
        if (error)
            *error = QStringLiteral("Unexpected response: no \"current\" object");
        return result;
    }
    const QJsonObject current = currentValue.toObject();

    const QTimeZone tehran("Asia/Tehran");
    const QDateTime now = QDateTime::currentDateTime();

    for (const WatchItem &w : kWatchList) {
        PriceItem item;
        item.key = QString::fromLatin1(w.key);
        item.label = QString::fromUtf8(w.label);

        if (current.contains(item.key)) {
            const QJsonObject o = current.value(item.key).toObject();

            const double sign = (o.value("dt").toString() == "low") ? -1.0 : 1.0;
            item.price = parseNumber(o.value("p").toString());
            item.change = sign * parseNumber(o.value("d").toString());
            item.percent = sign * o.value("dp").toDouble();

            item.updated = QDateTime::fromString(o.value("ts").toString(),
                                                 QStringLiteral("yyyy-MM-dd HH:mm:ss"));
            item.updated.setTimeZone(tehran);
            item.stale = !item.updated.isValid()
                         || item.updated.secsTo(now) > 24 * 3600;
            item.found = true;
        } else {
            item.stale = true;
        }
        result.append(item);
    }
    return result;
}