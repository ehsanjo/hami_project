#include "pricefetcher.h"
#include <QRandomGenerator>
#include <QUrlQuery>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimeZone>

#include "settings.h"

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

const char *const kCallSubdomains[] = {"call2", "call3", "call4"};
constexpr int kCallSubdomainCount = 3;

// Same as the site: a new random "rev" on every request, and one of the
// callN.tgju.org servers.
QUrl withFreshRev(QUrl url, int subdomain)
{
    if (!url.host().endsWith(QLatin1String("tgju.org")))
        return url;

    static const QRegularExpression callHost(QStringLiteral("^call\\d+\\.tgju\\.org$"));
    if (callHost.match(url.host()).hasMatch())
        url.setHost(QString::fromLatin1(kCallSubdomains[subdomain % kCallSubdomainCount])
                    + QStringLiteral(".tgju.org"));

    static const QString chars = QStringLiteral(
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
    QString rev;
    rev.reserve(60);
    for (int i = 0; i < 60; ++i)
        rev.append(chars.at(QRandomGenerator::global()->bounded(chars.size())));

    QUrlQuery query(url);
    query.removeAllQueryItems(QStringLiteral("rev"));
    query.addQueryItem(QStringLiteral("rev"), rev);
    url.setQuery(query);
    return url;
}

} // namespace

PriceFetcher::PriceFetcher(QObject *parent)
    : QObject(parent)
    , m_url(AppSettings::sourceUrl())
    , m_staleAfterHours(AppSettings::staleAfterHours())
{
    m_subdomain = int(QRandomGenerator::global()->bounded(kCallSubdomainCount));
}


void PriceFetcher::fetch()
{
    QNetworkRequest req(withFreshRev(m_url, m_subdomain));
    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute,
                     QNetworkRequest::AlwaysNetwork);
    req.setRawHeader("Cache-Control", "no-cache");
    req.setTransferTimeout(15000);

    const int staleHours = m_staleAfterHours;
    QNetworkReply *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, staleHours]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            QString message = reply->errorString();
            if (status > 0)
                message = QStringLiteral("HTTP %1 - %2").arg(status).arg(message);
            emit fetchFailed(message);
            return;
        }

        QString error;
        const QVector<PriceItem> items = parseResponse(reply->readAll(), &error, staleHours);
        if (!error.isEmpty()) {
            m_subdomain = (m_subdomain + 1) % kCallSubdomainCount;
            emit fetchFailed(error);
            return;
        }
        emit pricesReady(items);
    });
}

QVector<PriceItem> PriceFetcher::parseResponse(const QByteArray &body, QString *error,
                                               int staleAfterHours, const QDateTime &now)
{
    QVector<PriceItem> result;

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (error)
            *error = QStringLiteral("The response is not valid JSON (")
                     + parseError.errorString() + QLatin1Char(')');
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
    bool anyFound = false;

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
                         || item.updated.secsTo(now) > qint64(staleAfterHours) * 3600;
            item.found = true;
            anyFound = true;
        } else {
            item.stale = true;
        }
        result.append(item);
    }

    if (!anyFound) {
        if (error)
            *error = QStringLiteral("None of the expected prices were found "
                                    "(the site format or the URL may have changed)");
        return QVector<PriceItem>();
    }
    return result;
}