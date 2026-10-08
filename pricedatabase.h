#pragma once

#include <QDateTime>
#include <QString>
#include <QVector>

#include "pricefetcher.h"

struct PricePoint {
    QDateTime time; // UTC
    double price = 0.0;
};

class PriceDatabase
{
public:
    PriceDatabase();
    ~PriceDatabase();

    // Opens (and creates if needed) the database.
    // filePath empty = default file in the app data folder; ":memory:" is useful for tests.
    bool open(QString *error = nullptr, const QString &filePath = QString());
    bool isOpen() const { return m_open; }
    QString path() const { return m_path; }

    // Saves fresh items. Returns how many NEW rows were stored.
    int insertSnapshot(const QVector<PriceItem> &items);

    // All saved points for one item, oldest first.
    QVector<PricePoint> history(const QString &itemKey,
                                const QDateTime &since = QDateTime()) const;

    int totalRows() const;

private:
    Q_DISABLE_COPY(PriceDatabase)

    QString m_connName;
    QString m_path;
    bool m_open = false;
};