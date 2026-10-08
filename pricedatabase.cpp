#include "pricedatabase.h"

#include <QDebug>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>

PriceDatabase::PriceDatabase()
    : m_connName(QStringLiteral("pricehistory"))
{
}

PriceDatabase::~PriceDatabase()
{
    if (QSqlDatabase::contains(m_connName)) {
        {
            QSqlDatabase db = QSqlDatabase::database(m_connName, false);
            if (db.isOpen())
                db.close();
        } // db must be out of scope before removeDatabase
        QSqlDatabase::removeDatabase(m_connName);
    }
}

bool PriceDatabase::open(QString *error, const QString &filePath)
{
    if (filePath.isEmpty()) {
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (!QDir().mkpath(dir)) {
            if (error)
                *error = QStringLiteral("Cannot create folder: ") + dir;
            return false;
        }
        m_path = dir + QStringLiteral("/prices.sqlite");
    } else {
        m_path = filePath;
    }

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connName);
    db.setDatabaseName(m_path);
    if (!db.open()) {
        if (error)
            *error = db.lastError().text();
        return false;
    }

    QSqlQuery q(db);
    const bool ok = q.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS prices ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " item_key TEXT NOT NULL,"
        " ts TEXT NOT NULL,"      // site's update time, UTC, ISO format
        " price REAL NOT NULL,"
        " UNIQUE(item_key, ts))"));
    if (!ok) {
        if (error)
            *error = q.lastError().text();
        return false;
    }

    m_open = true;
    return true;
}

int PriceDatabase::insertSnapshot(const QVector<PriceItem> &items)
{
    if (!m_open)
        return 0;

    QSqlDatabase db = QSqlDatabase::database(m_connName);
    db.transaction();

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT OR IGNORE INTO prices (item_key, ts, price) "
        "VALUES (:key, :ts, :price)"));

    int inserted = 0;
    for (const PriceItem &it : items) {
        if (!it.found || it.stale || !it.updated.isValid())
            continue;

        q.bindValue(QStringLiteral(":key"), it.key);
        q.bindValue(QStringLiteral(":ts"), it.updated.toUTC().toString(Qt::ISODate));
        q.bindValue(QStringLiteral(":price"), it.price);

        if (!q.exec()) {
            qWarning() << "Insert failed:" << q.lastError().text();
            continue;
        }
        if (q.numRowsAffected() > 0)
            ++inserted;
    }

    db.commit();
    return inserted;
}

QVector<PricePoint> PriceDatabase::history(const QString &itemKey,
                                           const QDateTime &since) const
{
    QVector<PricePoint> points;
    if (!m_open)
        return points;

    const bool hasSince = since.isValid();

    QSqlDatabase db = QSqlDatabase::database(m_connName);
    QSqlQuery q(db);
    q.prepare(hasSince
                  ? QStringLiteral("SELECT ts, price FROM prices "
                                   "WHERE item_key = :key AND ts >= :since ORDER BY ts")
                  : QStringLiteral("SELECT ts, price FROM prices "
                                   "WHERE item_key = :key ORDER BY ts"));
    q.bindValue(QStringLiteral(":key"), itemKey);
    if (hasSince)
        q.bindValue(QStringLiteral(":since"), since.toUTC().toString(Qt::ISODate));

    if (!q.exec()) {
        qWarning() << "History query failed:" << q.lastError().text();
        return points;
    }
    while (q.next()) {
        PricePoint p;
        p.time = QDateTime::fromString(q.value(0).toString(), Qt::ISODate);
        p.price = q.value(1).toDouble();
        points.append(p);
    }
    return points;
}
int PriceDatabase::totalRows() const
{
    if (!m_open)
        return 0;
    QSqlQuery q(QSqlDatabase::database(m_connName));
    if (q.exec(QStringLiteral("SELECT COUNT(*) FROM prices")) && q.next())
        return q.value(0).toInt();
    return 0;
}