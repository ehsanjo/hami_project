#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimeZone>
#include <QtTest>

#include "alertmanager.h"
#include "analysis.h"
#include "pricedatabase.h"
#include "pricefetcher.h"
#include "settings.h"

namespace {

// Same shape as the real response, with just a few items.
const char *kSampleJson = R"({
  "current": {
    "geram18": {"p": "2,500,000", "d": "10,000", "dp": 0.4, "dt": "high", "ts": "2026-10-07 12:00:00"},
    "price_dollar_rl": {"p": "\t1,000,000", "d": "5,000", "dp": 0.5, "dt": "low", "ts": "2026-10-07 12:00:00"},
    "ons": {"p": "3,110.35", "d": "10.5", "dp": 0.3, "dt": "high", "ts": "2020-01-01 00:00:00"}
  }
})";

QDateTime tehranTime(const QString &text)
{
    QDateTime dt = QDateTime::fromString(text, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    dt.setTimeZone(QTimeZone("Asia/Tehran"));
    return dt;
}

PriceItem item(const char *key, double price, bool stale = false)
{
    PriceItem it;
    it.key = QString::fromLatin1(key);
    it.label = it.key;
    it.price = price;
    it.found = true;
    it.stale = stale;
    it.updated = QDateTime::currentDateTime();
    return it;
}

const PriceItem *findItem(const QVector<PriceItem> &items, const QString &key)
{
    for (const PriceItem &it : items) {
        if (it.key == key)
            return &it;
    }
    return nullptr;
}

} // namespace

class TstCore : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_settingsDir;

private slots:
    void initTestCase()
    {
        // Keep the tests away from the real settings of the app
        QCoreApplication::setOrganizationName(QStringLiteral("PriceTrackerTest"));
        QCoreApplication::setApplicationName(QStringLiteral("tst_core"));
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir.path());
    }

    void init()
    {
        QSettings().clear();
    }

    // ---------- Parsing ----------
    void parse_basic()
    {
        QString error;
        const QDateTime now = tehranTime(QStringLiteral("2026-10-07 13:00:00"));
        const QVector<PriceItem> items =
            PriceFetcher::parseResponse(QByteArray(kSampleJson), &error, 24, now);
        QVERIFY2(error.isEmpty(), qPrintable(error));

        const PriceItem *gold = findItem(items, QStringLiteral("geram18"));
        QVERIFY(gold && gold->found);
        QCOMPARE(gold->price, 2500000.0);
        QCOMPARE(gold->change, 10000.0);   // "high" = up
        QVERIFY(!gold->stale);

        const PriceItem *dollar = findItem(items, QStringLiteral("price_dollar_rl"));
        QVERIFY(dollar && dollar->found);
        QCOMPARE(dollar->price, 1000000.0); // tab and commas removed
        QCOMPARE(dollar->change, -5000.0);  // "low" = down
        QCOMPARE(dollar->percent, -0.5);

        const PriceItem *ounce = findItem(items, QStringLiteral("ons"));
        QVERIFY(ounce && ounce->found);
        QVERIFY(ounce->stale);              // timestamp from 2020

        const PriceItem *missing = findItem(items, QStringLiteral("geram24"));
        QVERIFY(missing && !missing->found);
    }

    void parse_errors()
    {
        QString notJson;
        QVERIFY(PriceFetcher::parseResponse(QByteArray("not json"), &notJson).isEmpty());
        QVERIFY(!notJson.isEmpty());

        QString noCurrent;
        QVERIFY(PriceFetcher::parseResponse(QByteArray(R"({"x": 1})"), &noCurrent).isEmpty());
        QVERIFY(!noCurrent.isEmpty());

        QString noKnownItems;
        QVERIFY(PriceFetcher::parseResponse(
                    QByteArray(R"({"current": {"unknown": {"p": "1"}}})"), &noKnownItems).isEmpty());
        QVERIFY(!noKnownItems.isEmpty());
    }

    // ---------- Analysis ----------
    void stats()
    {
        QVector<PricePoint> points;
        for (double v : {100.0, 110.0, 90.0, 120.0}) {
            PricePoint p;
            p.time = QDateTime::currentDateTimeUtc();
            p.price = v;
            points.append(p);
        }
        const Analysis::PeriodStats s = Analysis::computeStats(points);
        QVERIFY(s.valid);
        QCOMPARE(s.points, 4);
        QCOMPARE(s.first, 100.0);
        QCOMPARE(s.last, 120.0);
        QCOMPARE(s.low, 90.0);
        QCOMPARE(s.high, 120.0);
        QCOMPARE(s.average, 105.0);
        QCOMPARE(s.changePercent, 20.0);

        QVERIFY(!Analysis::computeStats(QVector<PricePoint>()).valid);
        QVERIFY(!Analysis::computeStats(points.mid(0, 1)).valid); // one point is not enough
    }

    void goldRatios()
    {
        QVector<PriceItem> items{item("price_dollar_rl", 1000000.0),
                                 item("geram18", 100000000.0),
                                 item("geram24", 132000000.0),
                                 item("ons", 4000.0)};
        Analysis::GoldRatios r = Analysis::computeGoldRatios(items);
        QVERIFY(r.valid);
        QCOMPARE(r.usdPerGram18, 100.0);
        QCOMPARE(r.usdPerGram24, 132.0);
        QCOMPARE(r.gramsPer1000Usd, 10.0);
        QVERIFY(qAbs(r.globalUsdPerGram24 - 4000.0 / 31.1034768) < 1e-9);
        QVERIFY(qAbs(r.premiumPercent - (132.0 / (4000.0 / 31.1034768) - 1.0) * 100.0) < 1e-9);

        // A stale price makes the result invalid
        items[3].stale = true;
        QVERIFY(!Analysis::computeGoldRatios(items).valid);
    }

    // ---------- Alerts ----------
    void alerts_flow()
    {
        AlertManager manager;
        Alert a;
        a.key = QStringLiteral("geram18");
        a.label = QStringLiteral("Gold 18k");
        a.condition = Alert::Above;
        a.threshold = 100.0;
        manager.add(a);

        QSignalSpy spy(&manager, &AlertManager::alertTriggered);

        manager.check({item("geram18", 90.0)});
        QCOMPARE(spy.count(), 0);              // below the threshold
        manager.check({item("geram18", 100.0)});
        QCOMPARE(spy.count(), 1);              // reached it
        manager.check({item("geram18", 120.0)});
        QCOMPARE(spy.count(), 1);              // still true: no repeat
        manager.check({item("geram18", 80.0)});// condition false: re-arms
        manager.check({item("geram18", 101.0)});
        QCOMPARE(spy.count(), 2);              // fires again
        manager.check({item("geram18", 10.0, true)});
        QCOMPARE(spy.count(), 2);              // stale data is ignored
    }

    void alerts_persist()
    {
        {
            AlertManager first;
            Alert a;
            a.key = QStringLiteral("ons");
            a.label = QStringLiteral("Ounce");
            a.condition = Alert::Below;
            a.threshold = 3000.0;
            first.add(a);
        }

        AlertManager second; // loads from the settings file
        QCOMPARE(second.alerts().size(), 1);
        QCOMPARE(second.alerts().first().key, QStringLiteral("ons"));
        QVERIFY(second.alerts().first().condition == Alert::Below);
        QCOMPARE(second.alerts().first().threshold, 3000.0);

        second.removeAt(0);
        QCOMPARE(second.alerts().size(), 0);
    }

    // ---------- Database ----------
    void database()
    {
        PriceDatabase db;
        QString error;
        QVERIFY2(db.open(&error, QStringLiteral(":memory:")), qPrintable(error));

        PriceItem gold = item("geram18", 100.0);
        gold.updated = tehranTime(QStringLiteral("2026-10-07 12:00:00"));
        QVector<PriceItem> items{gold};

        QCOMPARE(db.insertSnapshot(items), 1);
        QCOMPARE(db.insertSnapshot(items), 0);  // same site timestamp: ignored

        items[0].updated = tehranTime(QStringLiteral("2026-10-07 12:01:00"));
        items[0].price = 101.0;
        QCOMPARE(db.insertSnapshot(items), 1);

        QVector<PriceItem> staleOnly{item("ons", 50.0, true)};
        QCOMPARE(db.insertSnapshot(staleOnly), 0); // stale data is never saved

        QCOMPARE(db.totalRows(), 2);

        const QVector<PricePoint> history = db.history(QStringLiteral("geram18"));
        QCOMPARE(history.size(), 2);
        QCOMPARE(history.at(0).price, 100.0);   // oldest first
        QCOMPARE(history.at(1).price, 101.0);

        QCOMPARE(db.history(QStringLiteral("geram18"), items[0].updated).size(), 1);
    }

    // ---------- Settings ----------
    void settings_roundtrip()
    {
        QCOMPARE(AppSettings::sourceUrl(), AppSettings::defaultSourceUrl());

        AppSettings::setSourceUrl(QStringLiteral("https://example.com/a.json"));
        QCOMPARE(AppSettings::sourceUrl(), QStringLiteral("https://example.com/a.json"));

        AppSettings::setStaleAfterHours(5000);   // clamped
        QCOMPARE(AppSettings::staleAfterHours(), 720);

        AppSettings::setRefreshIntervalSeconds(1); // clamped
        QCOMPARE(AppSettings::refreshIntervalSeconds(), 30);

        AppSettings::setAutoRefresh(false);
        QVERIFY(!AppSettings::autoRefresh());
    }
};

QTEST_GUILESS_MAIN(TstCore)
#include "tst_core.moc"