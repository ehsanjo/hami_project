#include "analysiswidget.h"

#include <QColor>
#include <QDateTime>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLocale>
#include <QTableWidget>
#include <QVBoxLayout>

#include "analysis.h"
#include "pricedatabase.h"

namespace {

QString formatNumber(double value, bool alwaysSign = false)
{
    static const QLocale english(QLocale::English, QLocale::UnitedStates);
    const int decimals = qAbs(value) < 100000 ? 2 : 0;
    QString text = english.toString(value, 'f', decimals);
    if (alwaysSign && value > 0)
        text.prepend('+');
    return text;
}

QTableWidgetItem *makeItem(const QString &text, bool alignRight = true)
{
    auto *item = new QTableWidgetItem(text);
    item->setTextAlignment((alignRight ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignVCenter);
    return item;
}

QTableWidgetItem *percentItem(const Analysis::PeriodStats &s)
{
    if (!s.valid)
        return makeItem(QStringLiteral("-"));
    auto *item = makeItem(formatNumber(s.changePercent, true) + QStringLiteral("%"));
    if (s.changePercent > 0)
        item->setForeground(QColor(0, 140, 0));
    else if (s.changePercent < 0)
        item->setForeground(QColor(200, 0, 0));
    return item;
}

QTableWidgetItem *valueItem(const Analysis::PeriodStats &s, double value)
{
    return makeItem(s.valid ? formatNumber(value) : QStringLiteral("-"));
}

} // namespace

AnalysisWidget::AnalysisWidget(PriceDatabase *db, QWidget *parent)
    : QWidget(parent)
    , m_db(db)
{
    auto *layout = new QVBoxLayout(this);

    // Gold vs dollar box
    auto *box = new QGroupBox(QStringLiteral("Gold vs dollar (latest prices)"));
    auto *form = new QFormLayout(box);
    m_usd18 = new QLabel(QStringLiteral("-"));
    m_usd24 = new QLabel(QStringLiteral("-"));
    m_global = new QLabel(QStringLiteral("-"));
    m_premium = new QLabel(QStringLiteral("-"));
    m_per1000 = new QLabel(QStringLiteral("-"));
    form->addRow(QStringLiteral("Local gold 18k, 1 gram:"), m_usd18);
    form->addRow(QStringLiteral("Local gold 24k, 1 gram:"), m_usd24);
    form->addRow(QStringLiteral("Global gold, 1 gram (ounce / 31.1035):"), m_global);
    form->addRow(QStringLiteral("Local 24k vs global:"), m_premium);
    form->addRow(QStringLiteral("1,000 USD buys (18k gold):"), m_per1000);
    layout->addWidget(box);

    // Statistics table
    m_table = new QTableWidget(0, 8);
    m_table->setHorizontalHeaderLabels({QStringLiteral("Name"),
                                        QStringLiteral("Latest"),
                                        QStringLiteral("24h change"),
                                        QStringLiteral("24h average"),
                                        QStringLiteral("24h low"),
                                        QStringLiteral("24h high"),
                                        QStringLiteral("7d change"),
                                        QStringLiteral("7d average")});
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->verticalHeader()->hide();
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    layout->addWidget(m_table, 1);

    auto *note = new QLabel(QStringLiteral(
        "Statistics are calculated from the saved history, so they become more "
        "meaningful the longer the app has been running. \"-\" means fewer than "
        "two saved points in that period. The local vs global difference is only a "
        "rough indicator: prices update at different moments and local prices include "
        "fees and taxes."));
    note->setWordWrap(true);
    layout->addWidget(note);
}

void AnalysisWidget::refresh(const QVector<PriceItem> &items)
{
    // ---- Gold vs dollar ----
    const Analysis::GoldRatios r = Analysis::computeGoldRatios(items);
    if (r.valid) {
        m_usd18->setText(formatNumber(r.usdPerGram18) + QStringLiteral(" USD"));
        m_usd24->setText(formatNumber(r.usdPerGram24) + QStringLiteral(" USD"));
        m_global->setText(formatNumber(r.globalUsdPerGram24) + QStringLiteral(" USD"));
        m_premium->setText(formatNumber(r.premiumPercent, true) + QStringLiteral("%"));
        m_per1000->setText(formatNumber(r.gramsPer1000Usd) + QStringLiteral(" g"));
    } else {
        for (QLabel *label : {m_usd18, m_usd24, m_global, m_premium, m_per1000})
            label->setText(QStringLiteral("- (needs fresh dollar, gold and ounce prices)"));
    }

    // ---- Statistics ----
    m_table->setRowCount(items.size());
    const QDateTime now = QDateTime::currentDateTimeUtc();

    for (int row = 0; row < items.size(); ++row) {
        const PriceItem &it = items.at(row);

        Analysis::PeriodStats day;
        Analysis::PeriodStats week;
        if (m_db && m_db->isOpen() && it.found) {
            day = Analysis::computeStats(m_db->history(it.key, now.addDays(-1)));
            week = Analysis::computeStats(m_db->history(it.key, now.addDays(-7)));
        }

        m_table->setItem(row, 0, makeItem(it.label, false));
        m_table->setItem(row, 1, makeItem(it.found ? formatNumber(it.price) : QStringLiteral("-")));
        m_table->setItem(row, 2, percentItem(day));
        m_table->setItem(row, 3, valueItem(day, day.average));
        m_table->setItem(row, 4, valueItem(day, day.low));
        m_table->setItem(row, 5, valueItem(day, day.high));
        m_table->setItem(row, 6, percentItem(week));
        m_table->setItem(row, 7, valueItem(week, week.average));
    }
}