#pragma once

#include <QObject>
#include <QString>
#include <QVector>

#include "pricefetcher.h"

struct Alert {
    enum Condition { Above = 0, Below = 1 };

    QString key;    // item key, e.g. "geram18"
    QString label;  // display name
    Condition condition = Above;
    double threshold = 0.0;
    bool fired = false; // true after it triggered, until the condition is false again
};

class AlertManager : public QObject
{
    Q_OBJECT
public:
    explicit AlertManager(QObject *parent = nullptr);

    const QVector<Alert> &alerts() const { return m_alerts; }

    void add(const Alert &alert);
    void removeAt(int index);

    // Call after every successful price update.
    void check(const QVector<PriceItem> &items);

signals:
    void alertTriggered(const Alert &alert, double price);
    void alertsChanged();

private:
    void load();
    void save() const;

    QVector<Alert> m_alerts;
};