#include "settingswidget.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QUrl>
#include <QVBoxLayout>

#include "settings.h"

SettingsWidget::SettingsWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);

    auto *form = new QFormLayout;
    m_urlEdit = new QLineEdit;
    m_urlEdit->setPlaceholderText(QStringLiteral("https://..."));
    m_staleSpin = new QSpinBox;
    m_staleSpin->setRange(1, 720);
    m_staleSpin->setSuffix(QStringLiteral(" hours"));
    form->addRow(QStringLiteral("Data source URL:"), m_urlEdit);
    form->addRow(QStringLiteral("Treat a price as stale after:"), m_staleSpin);
    layout->addLayout(form);

    auto *buttons = new QHBoxLayout;
    auto *saveButton = new QPushButton(QStringLiteral("Save and refresh"));
    auto *resetButton = new QPushButton(QStringLiteral("Reset to defaults"));
    buttons->addWidget(saveButton);
    buttons->addWidget(resetButton);
    buttons->addStretch();
    layout->addLayout(buttons);

    m_message = new QLabel;
    m_message->setWordWrap(true);
    layout->addWidget(m_message);

    auto *note = new QLabel(QStringLiteral(
                                "If the prices stop updating, the site may have changed its address or token "
                                "(the rev=... part). Open the site in your browser, find the request again with "
                                "the Network tab, and paste the new URL here.\n\n"
                                "The refresh interval and auto refresh are set on the Live prices tab and are "
                                "remembered automatically.\n\n"
                                "Settings file: ") + QSettings().fileName());
    note->setWordWrap(true);
    note->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(note);
    layout->addStretch();

    connect(saveButton, &QPushButton::clicked, this, &SettingsWidget::apply);
    connect(resetButton, &QPushButton::clicked, this, &SettingsWidget::resetDefaults);

    loadValues();
}

void SettingsWidget::loadValues()
{
    m_urlEdit->setText(AppSettings::sourceUrl());
    m_staleSpin->setValue(AppSettings::staleAfterHours());
}

void SettingsWidget::apply()
{
    const QString text = m_urlEdit->text().trimmed();
    const QUrl url(text, QUrl::StrictMode);
    const bool ok = url.isValid() && !url.host().isEmpty()
                    && (url.scheme() == QLatin1String("http")
                        || url.scheme() == QLatin1String("https"));
    if (!ok) {
        m_message->setStyleSheet(QStringLiteral("color: #c00000;"));
        m_message->setText(QStringLiteral("Please enter a valid http:// or https:// address."));
        return;
    }

    AppSettings::setSourceUrl(text);
    AppSettings::setStaleAfterHours(m_staleSpin->value());

    m_message->setStyleSheet(QStringLiteral("color: #008000;"));
    m_message->setText(QStringLiteral("Saved. Fetching prices with the new settings..."));
    emit settingsApplied();
}

void SettingsWidget::resetDefaults()
{
    AppSettings::setSourceUrl(AppSettings::defaultSourceUrl());
    AppSettings::setStaleAfterHours(24);
    loadValues();

    m_message->setStyleSheet(QStringLiteral("color: #008000;"));
    m_message->setText(QStringLiteral("Defaults restored. Fetching prices..."));
    emit settingsApplied();
}