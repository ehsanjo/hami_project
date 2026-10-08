#include "alertswidget.h"

#include <QColor>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFont>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QVBoxLayout>

#include "alertmanager.h"

namespace {

QString formatNumber(double value)
{
    static const QLocale english(QLocale::English, QLocale::UnitedStates);
    return english.toString(value, 'f', qAbs(value) < 100000 ? 2 : 0);
}

QTableWidgetItem *makeItem(const QString &text, bool alignRight = true)
{
    auto *item = new QTableWidgetItem(text);
    item->setTextAlignment((alignRight ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignVCenter);
    return item;
}

} // namespace

AlertsWidget::AlertsWidget(AlertManager *manager, QWidget *parent)
    : QWidget(parent)
    , m_manager(manager)
{
    auto *layout = new QVBoxLayout(this);

    // ---- New alert form ----
    auto *box = new QGroupBox(QStringLiteral("New alert"));
    auto *boxLayout = new QVBoxLayout(box);

    auto *row = new QHBoxLayout;
    m_itemCombo = new QComboBox;
    m_conditionCombo = new QComboBox;
    m_conditionCombo->addItem(QStringLiteral("rises to or above"), static_cast<int>(Alert::Above));
    m_conditionCombo->addItem(QStringLiteral("falls to or below"), static_cast<int>(Alert::Below));
    m_thresholdSpin = new QDoubleSpinBox;
    m_thresholdSpin->setRange(0.0, 1e13);
    m_thresholdSpin->setDecimals(2);
    m_thresholdSpin->setGroupSeparatorShown(true);
    m_addButton = new QPushButton(QStringLiteral("Add alert"));

    row->addWidget(new QLabel(QStringLiteral("Notify me when")));
    row->addWidget(m_itemCombo, 1);
    row->addWidget(m_conditionCombo);
    row->addWidget(m_thresholdSpin, 1);
    row->addWidget(m_addButton);
    boxLayout->addLayout(row);

    m_currentLabel = new QLabel(QStringLiteral("Current price: -"));
    boxLayout->addWidget(m_currentLabel);
    layout->addWidget(box);

    // ---- Alerts table ----
    m_table = new QTableWidget(0, 5);
    m_table->setHorizontalHeaderLabels({QStringLiteral("Item"),
                                        QStringLiteral("Condition"),
                                        QStringLiteral("Price"),
                                        QStringLiteral("Current"),
                                        QStringLiteral("Status")});
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->verticalHeader()->hide();
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    layout->addWidget(m_table, 1);

    m_removeButton = new QPushButton(QStringLiteral("Remove selected"));
    layout->addWidget(m_removeButton, 0, Qt::AlignRight);

    auto *note = new QLabel(QStringLiteral(
        "Alerts are checked after every price update. An alert triggers once when its "
        "condition becomes true and re-arms when the condition is false again. An alert "
        "that is already true when you add it triggers at the next update. Prices of "
        "most items are in rial; the gold ounce is in USD."));
    note->setWordWrap(true);
    layout->addWidget(note);

    connect(m_addButton, &QPushButton::clicked, this, &AlertsWidget::addAlert);
    connect(m_removeButton, &QPushButton::clicked, this, &AlertsWidget::removeSelected);
    connect(m_itemCombo, &QComboBox::currentIndexChanged, this, &AlertsWidget::onItemChanged);
    connect(m_manager, &AlertManager::alertsChanged, this, &AlertsWidget::rebuildTable);

    rebuildTable();
}

void AlertsWidget::refresh(const QVector<PriceItem> &items)
{
    m_latest.clear();
    for (const PriceItem &it : items) {
        if (it.found && !it.stale)
            m_latest.insert(it.key, it.price);
    }

    // Fill the item list once
    if (m_itemCombo->count() == 0) {
        {
            const QSignalBlocker blocker(m_itemCombo);
            for (const PriceItem &it : items)
                m_itemCombo->addItem(it.label, it.key);
        }
        onItemChanged(); // sets the default price from the current price
    } else {
        updateCurrentLabel();
    }

    rebuildTable();
}

void AlertsWidget::onItemChanged()
{
    const QString key = m_itemCombo->currentData().toString();
    if (m_latest.contains(key))
        m_thresholdSpin->setValue(m_latest.value(key)); // handy starting point
    updateCurrentLabel();
}

void AlertsWidget::updateCurrentLabel()
{
    const QString key = m_itemCombo->currentData().toString();
    m_currentLabel->setText(QStringLiteral("Current price: ")
                            + (m_latest.contains(key) ? formatNumber(m_latest.value(key))
                                                      : QStringLiteral("-")));
}

void AlertsWidget::addAlert()
{
    if (m_itemCombo->currentIndex() < 0)
        return;

    Alert a;
    a.key = m_itemCombo->currentData().toString();
    a.label = m_itemCombo->currentText();
    a.condition = static_cast<Alert::Condition>(m_conditionCombo->currentData().toInt());
    a.threshold = m_thresholdSpin->value();
    m_manager->add(a); // emits alertsChanged -> table is rebuilt
}

void AlertsWidget::removeSelected()
{
    const int row = m_table->currentRow();
    if (row >= 0)
        m_manager->removeAt(row);
}

void AlertsWidget::rebuildTable()
{
    const QVector<Alert> &alerts = m_manager->alerts();
    m_table->setRowCount(alerts.size());

    for (int i = 0; i < alerts.size(); ++i) {
        const Alert &a = alerts.at(i);

        m_table->setItem(i, 0, makeItem(a.label, false));
        m_table->setItem(i, 1, makeItem(a.condition == Alert::Above
                                            ? QStringLiteral("at or above")
                                            : QStringLiteral("at or below"), false));
        m_table->setItem(i, 2, makeItem(formatNumber(a.threshold)));
        m_table->setItem(i, 3, makeItem(m_latest.contains(a.key)
                                            ? formatNumber(m_latest.value(a.key))
                                            : QStringLiteral("-")));

        auto *status = makeItem(a.fired ? QStringLiteral("Triggered")
                                        : QStringLiteral("Waiting"), false);
        if (a.fired) {
            status->setForeground(QColor(200, 110, 0));
            QFont f = status->font();
            f.setBold(true);
            status->setFont(f);
        }
        m_table->setItem(i, 4, status);
    }
}