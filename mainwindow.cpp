#include "mainwindow.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QColor>
#include <QComboBox>
#include <QScroller>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QLocale>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QSpinBox>
#include <QStatusBar>
#include <QSystemTrayIcon>
#include <QTabWidget>
#include <QTableWidget>
#include <QTime>
#include <QTimeZone>
#include <QTimer>
#include <QVBoxLayout>

#include "alertswidget.h"
#include "analysiswidget.h"
#include "chartwidget.h"
#include "settings.h"
#include "settingswidget.h"

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

// A simple gold coin drawn in code, so no image files are needed.
QIcon makeAppIcon()
{
    QPixmap pm(64, 64);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(QColor(150, 115, 20), 3));
    p.setBrush(QColor(230, 190, 50));
    p.drawEllipse(4, 4, 56, 56);

    QFont f = p.font();
    f.setBold(true);
    f.setPixelSize(36);
    p.setFont(f);
    p.setPen(QColor(110, 80, 10));
    p.drawText(pm.rect(), Qt::AlignCenter, QStringLiteral("$"));
    p.end();

    return QIcon(pm);
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_timer(new QTimer(this))
{
    setWindowTitle(QStringLiteral("Gold & Dollar Tracker"));
    setWindowIcon(makeAppIcon());

    auto *tabs = new QTabWidget(this);
#ifdef Q_OS_ANDROID
    // Phone layout: thumb-friendly tab bar at the bottom, scroll arrows for 6 tabs
    tabs->setTabPosition(QTabWidget::South);
    tabs->setUsesScrollButtons(true);
#endif

    // ---------- Live prices page ----------
    auto *livePage = new QWidget;
    auto *liveLayout = new QVBoxLayout(livePage);

    auto *bar = new QHBoxLayout;
    m_refreshButton = new QPushButton(QStringLiteral("Refresh now"));
    m_autoCheck = new QCheckBox(QStringLiteral("Auto refresh every"));
    m_intervalSpin = new QSpinBox;
    m_intervalSpin->setRange(10, 3600); // be polite to the server
    m_intervalSpin->setSuffix(QStringLiteral(" s"));
    // Remembered between runs
    m_autoCheck->setChecked(AppSettings::autoRefresh());
    m_intervalSpin->setValue(AppSettings::refreshIntervalSeconds());
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
#ifdef Q_OS_ANDROID
    // Portrait phone screens are narrow: hide "Change" and "Updated".
    // Stale rows still turn gray. Delete these two lines to show all columns.
    m_table->setColumnHidden(2, true);
    m_table->setColumnHidden(4, true);
    m_table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    QScroller::grabGesture(m_table->viewport(), QScroller::LeftMouseButtonGesture);
#endif
    tabs->addTab(livePage, QStringLiteral("Live prices"));

    // ---------- Chart page ----------
    m_chartWidget = new ChartWidget(&m_db, this);
    tabs->addTab(m_chartWidget, QStringLiteral("Chart"));

    // ---------- Analysis page ----------
    m_analysisWidget = new AnalysisWidget(&m_db, this);
    tabs->addTab(m_analysisWidget, QStringLiteral("Analysis"));

    // ---------- Alerts page ----------
    m_alertsWidget = new AlertsWidget(&m_alertManager, this);
    tabs->addTab(m_alertsWidget, QStringLiteral("Alerts"));

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
#ifdef Q_OS_ANDROID
    m_historyTable->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    QScroller::grabGesture(m_historyTable->viewport(), QScroller::LeftMouseButtonGesture);
#endif

    m_historyInfo = new QLabel;
    m_historyInfo->setTextInteractionFlags(Qt::TextSelectableByMouse);
    historyLayout->addWidget(m_historyInfo);

    tabs->addTab(historyPage, QStringLiteral("History"));

    // ---------- Settings page ----------
    m_settingsWidget = new SettingsWidget(this);
    tabs->addTab(m_settingsWidget, QStringLiteral("Settings"));

    setCentralWidget(tabs);

    m_statusLabel = new QLabel(QStringLiteral("Starting..."));
    statusBar()->addWidget(m_statusLabel);

    // ---------- Database ----------
    QString dbError;
    if (!m_db.open(&dbError))
        m_historyInfo->setText(QStringLiteral("Database error: ") + dbError
                               + QStringLiteral("  (prices will not be saved)"));

    // ---------- Tray icon ----------
    setupTray();

    // ---------- Connections ----------
    connect(m_refreshButton, &QPushButton::clicked, this, &MainWindow::refresh);
    connect(m_autoCheck, &QCheckBox::toggled, this, &MainWindow::applyTimerSettings);
    connect(m_intervalSpin, &QSpinBox::valueChanged, this, &MainWindow::applyTimerSettings);
    connect(m_timer, &QTimer::timeout, this, &MainWindow::refresh);
    connect(&m_fetcher, &PriceFetcher::pricesReady, this, &MainWindow::onPricesReady);
    connect(&m_fetcher, &PriceFetcher::fetchFailed, this, &MainWindow::onFetchFailed);
    connect(&m_alertManager, &AlertManager::alertTriggered, this, &MainWindow::onAlertTriggered);
    connect(m_settingsWidget, &SettingsWidget::settingsApplied, this, &MainWindow::onSettingsApplied);
    connect(m_historyCombo, &QComboBox::currentIndexChanged, this, &MainWindow::refreshHistory);
    connect(tabs, &QTabWidget::currentChanged, this, &MainWindow::refreshHistory);

    m_intervalSpin->setEnabled(m_autoCheck->isChecked());
    updateTimerInterval();
    QTimer::singleShot(0, this, &MainWindow::refresh); // first fetch right after startup
}

void MainWindow::setupTray()
{
#ifdef Q_OS_ANDROID
        return; // no system tray on Android: alerts use the in-app dialog
#endif
        if (!QSystemTrayIcon::isSystemTrayAvailable())
            return;
        // ... rest unchanged
        // no tray on this desktop: closing the window will quit the app

    m_tray = new QSystemTrayIcon(makeAppIcon(), this);
    m_tray->setToolTip(QStringLiteral("Gold & Dollar Tracker"));

    auto *menu = new QMenu(this);
    menu->addAction(QStringLiteral("Show / hide window"), this, &MainWindow::toggleVisibility);
    menu->addAction(QStringLiteral("Refresh now"), this, &MainWindow::refresh);
    menu->addSeparator();
    menu->addAction(QStringLiteral("Quit"), this, &MainWindow::quitApp);
    m_tray->setContextMenu(menu);

    connect(m_tray, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger)
            toggleVisibility();
    });

    m_tray->show();
    QApplication::setQuitOnLastWindowClosed(false);
}

void MainWindow::updateTrayTooltip()
{
    if (!m_tray)
        return;
    m_tray->setToolTip(m_failureCount >= 3
                           ? QStringLiteral("Gold & Dollar Tracker - offline")
                           : QStringLiteral("Gold & Dollar Tracker"));
}

void MainWindow::toggleVisibility()
{
    if (isVisible() && !isMinimized()) {
        hide();
    } else {
        showNormal();
        raise();
        activateWindow();
    }
}

void MainWindow::quitApp()
{
    QApplication::quit();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // With a working tray, closing the window only hides it.
    if (m_tray && m_tray->isVisible()) {
        hide();
        if (!m_trayHintShown) {
            m_tray->showMessage(QStringLiteral("Still running"),
                                QStringLiteral("Prices keep updating and alerts stay active. "
                                               "Use the tray icon menu to quit."),
                                QSystemTrayIcon::Information, 5000);
            m_trayHintShown = true;
        }
        event->ignore();
        return;
    }
    QMainWindow::closeEvent(event);
}

// Called when the user changes the auto refresh checkbox or the interval.
void MainWindow::applyTimerSettings()
{
    AppSettings::setAutoRefresh(m_autoCheck->isChecked());
    AppSettings::setRefreshIntervalSeconds(m_intervalSpin->value());
    m_intervalSpin->setEnabled(m_autoCheck->isChecked());
    updateTimerInterval();
}

// Sets the timer interval. After repeated failures it retries more slowly
// (2x, 4x, 8x the normal interval, at most 15 minutes).
void MainWindow::updateTimerInterval()
{
    const int base = m_intervalSpin->value();
    int seconds = base;
    if (m_failureCount > 1) {
        const int factor = 1 << qMin(m_failureCount - 1, 3);
        seconds = qMin(base * factor, qMax(base, 900));
    }
    m_timer->setInterval(seconds * 1000);
    if (m_autoCheck->isChecked())
        m_timer->start();
    else
        m_timer->stop();
}

void MainWindow::onSettingsApplied()
{
    m_fetcher.setUrl(QUrl(AppSettings::sourceUrl()));
    m_fetcher.setStaleAfterHours(AppSettings::staleAfterHours());
    m_failureCount = 0;
    updateTimerInterval();
    refresh();
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
    m_lastSuccess = QDateTime::currentDateTime();
    if (m_failureCount > 0) {
        m_failureCount = 0;
        updateTimerInterval(); // back to the normal interval
        updateTrayTooltip();
    }

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

    // Fill the item lists once
    if (m_historyCombo->count() == 0) {
        for (const PriceItem &it : items)
            m_historyCombo->addItem(it.label, it.key);
    }
    m_chartWidget->setItems(items);

    QString status = QStringLiteral("Last update: ")
                     + QTime::currentTime().toString(QStringLiteral("HH:mm:ss"));
    if (m_db.isOpen())
        status += QStringLiteral("  |  %1 new record(s) saved").arg(added);
    m_statusLabel->setText(status);

    refreshHistory();
    m_chartWidget->reload();
    m_analysisWidget->refresh(items);

    // Alerts: check first, then let the Alerts tab show the new state
    m_alertManager.check(items);
    m_alertsWidget->refresh(items);
}

void MainWindow::onFetchFailed(const QString &message)
{
    setBusy(false);
    ++m_failureCount;
    updateTimerInterval();
    updateTrayTooltip();

    QString text = QStringLiteral("Update failed: ") + message;
    if (m_failureCount > 1)
        text += QStringLiteral("  (%1 failures in a row, retrying more slowly)")
                    .arg(m_failureCount);
    if (m_lastSuccess.isValid())
        text += QStringLiteral("  |  showing data from ")
                + m_lastSuccess.toString(QStringLiteral("HH:mm:ss"));
    else
        text += QStringLiteral("  |  no data yet");
    m_statusLabel->setText(text);
}

void MainWindow::onAlertTriggered(const Alert &alert, double price)
{
    const QString direction = alert.condition == Alert::Above ? QStringLiteral("at or above")
                                                              : QStringLiteral("at or below");
    const QString title = QStringLiteral("Price alert");
    const QString text = QStringLiteral("%1 is now %2\n(alert: %3 %4)")
                             .arg(alert.label, formatNumber(price), direction,
                                  formatNumber(alert.threshold));

    if (m_tray && m_tray->isVisible() && QSystemTrayIcon::supportsMessages()) {
        m_tray->showMessage(title, text, QSystemTrayIcon::Information, 15000);
    } else {
        // No tray notification available: show a small non-blocking dialog
        auto *box = new QMessageBox(QMessageBox::Information, title, text,
                                    QMessageBox::Ok, this);
        box->setAttribute(Qt::WA_DeleteOnClose);
        box->open();
    }
    QApplication::beep();
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