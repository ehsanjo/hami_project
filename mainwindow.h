#pragma once

#include <QDateTime>
#include <QMainWindow>
#include <QVector>

#include "alertmanager.h"
#include "pricedatabase.h"
#include "pricefetcher.h"

class AlertsWidget;
class AnalysisWidget;
class ChartWidget;
class QCheckBox;
class QCloseEvent;
class QComboBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QSystemTrayIcon;
class QTableWidget;
class QTimer;
class SettingsWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void refresh();
    void onPricesReady(const QVector<PriceItem> &items);
    void onFetchFailed(const QString &message);
    void onAlertTriggered(const Alert &alert, double price);
    void onSettingsApplied();
    void applyTimerSettings();
    void refreshHistory();
    void toggleVisibility();
    void quitApp();

private:
    void setBusy(bool busy);
    void setupTray();
    void updateTimerInterval();
    void updateTrayTooltip();

    PriceFetcher m_fetcher;
    PriceDatabase m_db;
    AlertManager m_alertManager;
    QTimer *m_timer;
    QTableWidget *m_table;
    QPushButton *m_refreshButton;
    QCheckBox *m_autoCheck;
    QSpinBox *m_intervalSpin;
    QLabel *m_statusLabel;
    QComboBox *m_historyCombo;
    QTableWidget *m_historyTable;
    QLabel *m_historyInfo;
    ChartWidget *m_chartWidget;
    AnalysisWidget *m_analysisWidget;
    AlertsWidget *m_alertsWidget;
    SettingsWidget *m_settingsWidget;
    QSystemTrayIcon *m_tray = nullptr;
    QDateTime m_lastSuccess;
    int m_failureCount = 0;
    bool m_trayHintShown = false;
    bool m_busy = false;
};