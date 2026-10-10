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
    bool stale = false;// older than the stale limit, or unreadable time
    bool found = false;// false if the key was missing in the response
};

class PriceFetcher : public QObject
{
    Q_OBJECT
public:
    explicit PriceFetcher(QObject *parent = nullptr);

    void setUrl(const QUrl &url) { m_url = url; }
    void setStaleAfterHours(int hours) { m_staleAfterHours = hours; }

    // Starts one asynchronous request. Result arrives by signal.
    void fetch();

    // Pure parsing function (no network), easy to unit-test.
    // On any problem it returns an empty vector and fills *error.
    static QVector<PriceItem> parseResponse(const QByteArray &body,
                                            QString *error = nullptr,
                                            int staleAfterHours = 24,
                                            const QDateTime &now = QDateTime::currentDateTime());
    static PriceItem costummize_prices(const QVector<PriceItem> &items);

signals:
    void pricesReady(const QVector<PriceItem> &items);
    void fetchFailed(const QString &message);

private:
    QNetworkAccessManager m_nam;
    QUrl m_url;
    int m_staleAfterHours = 24;
    int m_subdomain = 0; // index of the callN server currently used
};