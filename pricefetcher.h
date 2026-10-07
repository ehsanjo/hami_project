#pragma once

#include <QDateTime>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QVector>

// One parsed price entry. change and percent are signed:
// positive = price went up, negative = went down.
struct PriceItem {
    QString key;       // key in the website's JSON, e.g. "geram18"
    QString label;     // display name
    double price = 0.0;
    double change = 0.0;
    double percent = 0.0;
    QDateTime updated; // Tehran time
    bool stale = false;// older than 24 hours or unreadable time
    bool found = false;// false if the key was missing in the response
};

class PriceFetcher : public QObject
{
    Q_OBJECT
public:
    explicit PriceFetcher(QObject *parent = nullptr);

    // Starts one asynchronous request. Result arrives by signal.
    void fetch();

    // Pure parsing function (no network), easy to unit-test later.
    static QVector<PriceItem> parseResponse(const QByteArray &body,
                                            QString *error = nullptr);

signals:
    void pricesReady(const QVector<PriceItem> &items);
    void fetchFailed(const QString &message);

private:
    QNetworkAccessManager m_nam;
    QUrl m_url;
};