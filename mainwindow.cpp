#include "mainwindow.h"

#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QSpinBox>
#include <QStatusBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QTime>
#include <QTimeZone>
#include <QTimer>
#include <QVBoxLayout>

namespace {

// Thousands separators, e.g. 265,591,000
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

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_timer(new QTimer(this))
{
    setWindowTitle(QStringLiteral("Gold & Dollar Tracker"));

    auto *tabs = new QTabWidget(this);

    // ---------- Live prices page ----------
    auto *livePage = new QWidget;
    auto *liveLayout = new QVBoxLayout(livePage);

    auto *bar = new QHBoxLayout;
    m_refreshButton = new QPushButton(QStringLiteral("Refresh now"));
    m_autoCheck = new QCheckBox(QStringLiteral("Auto refresh every"));
    m_autoCheck->setChecked(true);
    m_intervalSpin = new QSpinBox;
    m_intervalSpin->setRange(30, 3600); // be polite to the server
    m_intervalSpin->setValue(60);
    m_intervalSpin->setSuffix(QStringLiteral(" s"));
    bar->addWidget(m_refreshButton);
    bar->addSpacing(12);
    bar->addWidget(m_autoCheck);
    bar->addWidget(m_intervalSpin);
    bar->addStretch();
    liveLayout->addLayout(bar);

    m_table = new QTableWidget(0, 5);
    m_table->setHorizontalHeaderLabels({QStringLiteral("Name"),
                                        QStringLiteral("Price"),
                                        QStringLiteral("Change"),
                                        QStringLiteral("Change %"),
                                        QStringLiteral("Updated (Tehran)")});
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->verticalHeader()->hide();
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    liveLayout->addWidget(m_table);

    tabs->addTab(livePage, QStringLiteral("Live prices"));

    // ---------- History page ----------
    auto *historyPage = new QWidget;
    auto *historyLayout = new QVBoxLayout(historyPage);

    auto *historyBar = new QHBoxLayout;
    historyBar->addWidget(new QLabel(QStringLiteral("Item:")));
    m_historyCombo = new QComboBox;
    historyBar->addWidget(m_historyCombo, 1);
    historyLayout->addLayout(historyBar);

    m_historyTable = new QTableWidget(0, 2);
    m_historyTable->setHorizontalHeaderLabels({QStringLiteral("Time (Tehran)"),
                                               QStringLiteral("Price")});
    m_historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_historyTable->verticalHeader()->hide();
    m_historyTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    historyLayout->addWidget(m_historyTable);

    m_historyInfo = new QLabel;
    m_historyInfo->setTextInteractionFlags(Qt::TextSelectableByMouse);
    historyLayout->addWidget(m_historyInfo);

    tabs->addTab(historyPage, QStringLiteral("History"));

    setCentralWidget(tabs);

    m_statusLabel = new QLabel(QStringLiteral("Starting..."));
    statusBar()->addWidget(m_statusLabel);

    // ---------- Database ----------
    QString dbError;
    if (!m_db.open(&dbError))
        m_historyInfo->setText(QStringLiteral("Database error: ") + dbError
                               + QStringLiteral("  (prices will not be saved)"));

    // ---------- Connections ----------
    connect(m_refreshButton, &QPushButton::clicked, this, &MainWindow::refresh);
    connect(m_autoCheck, &QCheckBox::toggled, this, &MainWindow::applyTimerSettings);
    connect(m_intervalSpin, &QSpinBox::valueChanged, this, &MainWindow::applyTimerSettings);
    connect(m_timer, &QTimer::timeout, this, &MainWindow::refresh);
    connect(&m_fetcher, &PriceFetcher::pricesReady, this, &MainWindow::onPricesReady);
    connect(&m_fetcher, &PriceFetcher::fetchFailed, this, &MainWindow::onFetchFailed);
    connect(m_historyCombo, &QComboBox::currentIndexChanged, this, &MainWindow::refreshHistory);
    connect(tabs, &QTabWidget::currentChanged, this, &MainWindow::refreshHistory);

    applyTimerSettings();
    QTimer::singleShot(0, this, &MainWindow::refresh); // first fetch right after startup
}

void MainWindow::applyTimerSettings()
{
    m_intervalSpin->setEnabled(m_autoCheck->isChecked());
    m_timer->setInterval(m_intervalSpin->value() * 1000);
    if (m_autoCheck->isChecked())
        m_timer->start();
    else
        m_timer->stop();
}

void MainWindow::setBusy(bool busy)
{
    m_busy = busy;
    m_refreshButton->setEnabled(!busy);
}

void MainWindow::refresh()
{
    if (m_busy)
        return; // a request is already running
    setBusy(true);
    m_statusLabel->setText(QStringLiteral("Updating..."));
    m_fetcher.fetch();
}

void MainWindow::onPricesReady(const QVector<PriceItem> &items)
{
    setBusy(false);
    m_table->setRowCount(items.size());

    const QColor up(0, 140, 0);
    const QColor down(200, 0, 0);
    const QColor gray(130, 130, 130);

    for (int row = 0; row < items.size(); ++row) {
        const PriceItem &it = items.at(row);

        auto *name = makeItem(it.label, false);
        auto *price = makeItem(it.found ? formatNumber(it.price) : QStringLiteral("-"));
        auto *change = makeItem(it.found ? formatNumber(it.change, true) : QStringLiteral("-"));
        auto *percent = makeItem(it.found
                                     ? formatNumber(it.percent, true) + QStringLiteral("%")
                                     : QStringLiteral("-"));
        QString updatedText = it.updated.isValid()
                                  ? it.updated.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))
                                  : QStringLiteral("-");
        if (it.stale)
            updatedText += QStringLiteral("  (stale)");
        auto *updated = makeItem(updatedText);

        if (it.stale) {
            for (auto *cell : {name, price, change, percent, updated})
                cell->setForeground(gray);
        } else if (it.change != 0.0) {
            const QColor c = it.change > 0 ? up : down;
            change->setForeground(c);
            percent->setForeground(c);
        }

        m_table->setItem(row, 0, name);
        m_table->setItem(row, 1, price);
        m_table->setItem(row, 2, change);
        m_table->setItem(row, 3, percent);
        m_table->setItem(row, 4, updated);
    }

    // Save to the database
    const int added = m_db.insertSnapshot(items);

    // Fill the history combo box once
    if (m_historyCombo->count() == 0) {
        for (const PriceItem &it : items)
            m_historyCombo->addItem(it.label, it.key);
    }

    QString status = QStringLiteral("Last update: ")
                     + QTime::currentTime().toString(QStringLiteral("HH:mm:ss"));
    if (m_db.isOpen())
        status += QStringLiteral("  |  %1 new record(s) saved").arg(added);
    m_statusLabel->setText(status);

    refreshHistory();
}

void MainWindow::onFetchFailed(const QString &message)
{
    setBusy(false);
    m_statusLabel->setText(QStringLiteral("Update failed: ") + message
                           + QStringLiteral(" (showing previous data)"));
}

void MainWindow::refreshHistory()
{
    if (!m_db.isOpen())
        return;

    m_historyInfo->setText(QStringLiteral("Database: %1   |   %2 records in total")
                               .arg(m_db.path())
                               .arg(m_db.totalRows()));

    m_historyTable->setRowCount(0);
    const QString key = m_historyCombo->currentData().toString();
    if (key.isEmpty())
        return;

    static const QTimeZone tehran("Asia/Tehran");
    const QVector<PricePoint> points = m_db.history(key);
    const int shown = qMin(points.size(), 500); // newest 500, newest first
    m_historyTable->setRowCount(shown);

    for (int i = 0; i < shown; ++i) {
        const PricePoint &p = points.at(points.size() - 1 - i);
        m_historyTable->setItem(i, 0, makeItem(
                                          p.time.toTimeZone(tehran).toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")), false));
        m_historyTable->setItem(i, 1, makeItem(formatNumber(p.price)));
    }
}