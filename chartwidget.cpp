#include "chartwidget.h"

#include <QChart>
#include <QChartView>
#include <QComboBox>
#include <QDateTimeAxis>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineSeries>
#include <QLocale>
#include <QPainter>
#include <QSignalBlocker>
#include <QTimeZone>
#include <QValueAxis>
#include <QVBoxLayout>

#include "pricedatabase.h"

namespace {

QString formatNumber(double value)
{
    static const QLocale english(QLocale::English, QLocale::UnitedStates);
    return english.toString(value, 'f', qAbs(value) < 100000 ? 2 : 0);
}

} // namespace

ChartWidget::ChartWidget(PriceDatabase *db, QWidget *parent)
    : QWidget(parent)
    , m_db(db)
{
    auto *layout = new QVBoxLayout(this);

    // Top bar: item + time range
    auto *bar = new QHBoxLayout;
    bar->addWidget(new QLabel(QStringLiteral("Item:")));
    m_itemCombo = new QComboBox;
    bar->addWidget(m_itemCombo, 1);
    bar->addSpacing(12);
    bar->addWidget(new QLabel(QStringLiteral("Range:")));
    m_rangeCombo = new QComboBox;
    m_rangeCombo->addItem(QStringLiteral("Last hour"), 3600);
    m_rangeCombo->addItem(QStringLiteral("Last 24 hours"), 24 * 3600);
    m_rangeCombo->addItem(QStringLiteral("Last 7 days"), 7 * 24 * 3600);
    m_rangeCombo->addItem(QStringLiteral("Last 30 days"), 30 * 24 * 3600);
    m_rangeCombo->addItem(QStringLiteral("All"), 0);
    m_rangeCombo->setCurrentIndex(1);
    bar->addWidget(m_rangeCombo);
    layout->addLayout(bar);

    // Chart
    m_chart = new QChart;
    m_chart->legend()->hide();

    m_series = new QLineSeries;
    m_series->setPointsVisible(true);
    m_chart->addSeries(m_series);

    m_axisX = new QDateTimeAxis;
    m_axisX->setFormat(QStringLiteral("MM-dd HH:mm"));
    m_axisX->setTitleText(QStringLiteral("Time (Tehran)"));
    m_axisX->setTickCount(6);

    m_axisY = new QValueAxis;
    m_axisY->setTickCount(6);

    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_chart->addAxis(m_axisY, Qt::AlignLeft);
    m_series->attachAxis(m_axisX);
    m_series->attachAxis(m_axisY);

    m_view = new QChartView(m_chart); // the view owns the chart
    m_view->setRenderHint(QPainter::Antialiasing);
    layout->addWidget(m_view, 1);

    m_info = new QLabel;
    layout->addWidget(m_info);

    connect(m_itemCombo, &QComboBox::currentIndexChanged, this, &ChartWidget::reload);
    connect(m_rangeCombo, &QComboBox::currentIndexChanged, this, &ChartWidget::reload);
}

void ChartWidget::setItems(const QVector<PriceItem> &items)
{
    if (m_itemCombo->count() > 0)
        return;

    {
        const QSignalBlocker blocker(m_itemCombo);
        for (const PriceItem &it : items)
            m_itemCombo->addItem(it.label, it.key);
    }
    reload();
}

void ChartWidget::reload()
{
    m_series->clear();

    const QString key = m_itemCombo->currentData().toString();
    if (!m_db || !m_db->isOpen() || key.isEmpty()) {
        m_chart->setTitle(QString());
        m_info->setText(QStringLiteral("No data yet"));
        return;
    }

    const int seconds = m_rangeCombo->currentData().toInt();
    const QDateTime since = seconds > 0
                                ? QDateTime::currentDateTimeUtc().addSecs(-seconds)
                                : QDateTime();
    const QVector<PricePoint> points = m_db->history(key, since);

    m_chart->setTitle(m_itemCombo->currentText());

    if (points.isEmpty()) {
        m_info->setText(QStringLiteral("No saved data in this range yet. "
                                       "Leave the app running to collect history."));
        return;
    }

    static const QTimeZone tehran("Asia/Tehran");

    QVector<QPointF> chartPoints;
    chartPoints.reserve(points.size());
    double minY = points.first().price;
    double maxY = minY;
    qint64 firstX = 0;
    qint64 lastX = 0;

    for (int i = 0; i < points.size(); ++i) {
        const PricePoint &p = points.at(i);

        // Tehran wall-clock time, re-read as local time, so the axis shows Tehran time
        // whatever the computer's own time zone is.
        const QDateTime wall = p.time.toTimeZone(tehran);
        const QDateTime asLocal(wall.date(), wall.time());
        const qint64 x = asLocal.toMSecsSinceEpoch();

        chartPoints.append(QPointF(x, p.price));
        minY = qMin(minY, p.price);
        maxY = qMax(maxY, p.price);
        if (i == 0)
            firstX = x;
        lastX = x;
    }
    m_series->replace(chartPoints);

    // Axis ranges with a little padding
    const qint64 span = lastX - firstX;
    const qint64 padX = span > 0 ? span / 20 : 30 * 60 * 1000;
    m_axisX->setRange(QDateTime::fromMSecsSinceEpoch(firstX - padX),
                      QDateTime::fromMSecsSinceEpoch(lastX + padX));

    double padY = (maxY - minY) * 0.05;
    if (padY == 0.0)
        padY = qMax(1.0, qAbs(maxY) * 0.01);
    m_axisY->setRange(minY - padY, maxY + padY);
    m_axisY->setLabelFormat(maxY < 100000 ? QStringLiteral("%.2f") : QStringLiteral("%.0f"));

    m_info->setText(QStringLiteral("%1 point(s)   |   min %2   max %3   last %4")
                        .arg(points.size())
                        .arg(formatNumber(minY))
                        .arg(formatNumber(maxY))
                        .arg(formatNumber(points.last().price)));
}