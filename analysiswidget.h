#pragma once

#include <QVector>
#include <QWidget>

#include "pricefetcher.h"

class PriceDatabase;
class QLabel;
class QTableWidget;

class AnalysisWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AnalysisWidget(PriceDatabase *db, QWidget *parent = nullptr);

    // Recalculates everything from the latest items and the saved history.
    void refresh(const QVector<PriceItem> &items);

private:
    PriceDatabase *m_db;
    QLabel *m_usd18;
    QLabel *m_usd24;
    QLabel *m_global;
    QLabel *m_premium;
    QLabel *m_per1000;
    QTableWidget *m_table;
};