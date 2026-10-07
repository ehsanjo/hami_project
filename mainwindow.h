#pragma once

#include <QMainWindow>
#include <QVector>

#include "pricedatabase.h"
#include "pricefetcher.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QTableWidget;
class QTimer;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void refresh();
    void onPricesReady(const QVector<PriceItem> &items);
    void onFetchFailed(const QString &message);
    void applyTimerSettings();
    void refreshHistory();

private:
    void setBusy(bool busy);

    PriceFetcher m_fetcher;
    PriceDatabase m_db;
    QTimer *m_timer;
    QTableWidget *m_table;
    QPushButton *m_refreshButton;
    QCheckBox *m_autoCheck;
    QSpinBox *m_intervalSpin;
    QLabel *m_statusLabel;
    QComboBox *m_historyCombo;
    QTableWidget *m_historyTable;
    QLabel *m_historyInfo;
    bool m_busy = false;
};