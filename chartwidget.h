#pragma once

#include <QVector>
#include <QWidget>

#include "pricefetcher.h"

class PriceDatabase;
class QChart;
class QChartView;
class QComboBox;
class QDateTimeAxis;
class QLabel;
class QLineSeries;
class QValueAxis;

class ChartWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ChartWidget(PriceDatabase *db, QWidget *parent = nullptr);

    // Fills the item list (only the first call has an effect).
    void setItems(const QVector<PriceItem> &items);

public slots:
    // Reads the database again and redraws the chart.
    void reload();

private:
    PriceDatabase *m_db;
    QComboBox *m_itemCombo;
    QComboBox *m_rangeCombo;
    QChartView *m_view;
    QChart *m_chart;
    QLineSeries *m_series;
    QDateTimeAxis *m_axisX;
    QValueAxis *m_axisY;
    QLabel *m_info;
};