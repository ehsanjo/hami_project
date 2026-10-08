#include "alertmanager.h"

#include <QSettings>

AlertManager::AlertManager(QObject *parent)
    : QObject(parent)
{
    load();
}

void AlertManager::add(const Alert &alert)
{
    Alert a = alert;
    a.fired = false;
    m_alerts.append(a);
    save();
    emit alertsChanged();
}

void AlertManager::removeAt(int index)
{
    if (index < 0 || index >= m_alerts.size())
        return;
    m_alerts.removeAt(index);
    save();
    emit alertsChanged();
}

void AlertManager::check(const QVector<PriceItem> &items)
{
    bool changed = false;

    for (int i = 0; i < m_alerts.size(); ++i) {
        Alert &a = m_alerts[i];

        const PriceItem *item = nullptr;
        for (const PriceItem &it : items) {
            if (it.key == a.key) {
                item = &it;
                break;
            }
        }
        // Never trigger on missing or stale data
        if (!item || !item->found || item->stale)
            continue;

        const bool met = (a.condition == Alert::Above) ? item->price >= a.threshold
                                                       : item->price <= a.threshold;
        if (met && !a.fired) {
            a.fired = true;
            changed = true;
            const Alert copy = a;
            emit alertTriggered(copy, item->price);
        } else if (!met && a.fired) {
            a.fired = false; // re-arm
            changed = true;
        }
    }

    if (changed) {
        save();
        emit alertsChanged();
    }
}

void AlertManager::load()
{
    QSettings s;
    const int count = s.beginReadArray(QStringLiteral("alerts"));
    for (int i = 0; i < count; ++i) {
        s.setArrayIndex(i);
        Alert a;
        a.key = s.value(QStringLiteral("key")).toString();
        a.label = s.value(QStringLiteral("label")).toString();
        a.condition = static_cast<Alert::Condition>(s.value(QStringLiteral("condition"), 0).toInt());
        a.threshold = s.value(QStringLiteral("threshold")).toDouble();
        a.fired = s.value(QStringLiteral("fired"), false).toBool();
        if (!a.key.isEmpty())
            m_alerts.append(a);
    }
    s.endArray();
}

void AlertManager::save() const
{
    QSettings s;
    s.remove(QStringLiteral("alerts")); // clear old entries first
    s.beginWriteArray(QStringLiteral("alerts"), m_alerts.size());
    for (int i = 0; i < m_alerts.size(); ++i) {
        s.setArrayIndex(i);
        const Alert &a = m_alerts.at(i);
        s.setValue(QStringLiteral("key"), a.key);
        s.setValue(QStringLiteral("label"), a.label);
        s.setValue(QStringLiteral("condition"), static_cast<int>(a.condition));
        s.setValue(QStringLiteral("threshold"), a.threshold);
        s.setValue(QStringLiteral("fired"), a.fired);
    }
    s.endArray();
}