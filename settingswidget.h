#pragma once

#include <QWidget>

class QLabel;
class QLineEdit;
class QSpinBox;

class SettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsWidget(QWidget *parent = nullptr);

signals:
    // Emitted after new settings were saved. The main window reacts by reloading them.
    void settingsApplied();

private slots:
    void apply();
    void resetDefaults();

private:
    void loadValues();

    QLineEdit *m_urlEdit;
    QSpinBox *m_staleSpin;
    QLabel *m_message;
};