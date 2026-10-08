#pragma once

#include <QHash>
#include <QVector>
#include <QWidget>

#include "pricefetcher.h"

class AlertManager;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QTableWidget;

class AlertsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AlertsWidget(AlertManager *manager, QWidget *parent = nullptr);

    // Call after every price update.
    void refresh(const QVector<PriceItem> &items);

private slots:
    void addAlert();
    void removeSelected();
    void onItemChanged();
    void rebuildTable();

private:
    void updateCurrentLabel();

    AlertManager *m_manager;
    QComboBox *m_itemCombo;
    QComboBox *m_conditionCombo;
    QDoubleSpinBox *m_thresholdSpin;
    QLabel *m_currentLabel;
    QPushButton *m_addButton;
    QPushButton *m_removeButton;
    QTableWidget *m_table;
    QHash<QString, double> m_latest; // fresh prices by item key
};