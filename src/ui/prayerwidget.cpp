#include "prayerwidget.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

PrayerWidget::PrayerWidget(const QString &name, const QString &nameAr,
                           const QString &time, QWidget *parent)
    : QWidget(parent)
    , m_name(name)
    , m_nameAr(nameAr)
    , m_time(time)
{
    setObjectName("prayerRow");
    setupUi();
}

void PrayerWidget::setupUi()
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 2, 0, 2);

    m_nameLabel = new QLabel;
    m_nameLabel->setObjectName("prayerName");
    m_nameLabel->setText(m_name + " (" + m_nameAr + ")");

    m_timeLabel = new QLabel;
    m_timeLabel->setObjectName("prayerTime");
    m_timeLabel->setText(m_time);
    m_timeLabel->setAlignment(Qt::AlignCenter);

    m_adhanBtn = new QPushButton;
    m_adhanBtn->setObjectName("adhanToggle");
    m_adhanBtn->setFixedSize(28, 28);
    m_adhanBtn->setCheckable(true);
    m_adhanBtn->setChecked(m_adhanEnabled);
    m_adhanBtn->setCursor(Qt::PointingHandCursor);

    connect(m_adhanBtn, &QPushButton::toggled, this, [this](bool checked) {
        m_adhanEnabled = checked;
        m_adhanBtn->setText(checked ? "🔊" : "🔇");
        emit adhanToggled(m_name, checked);
    });
    m_adhanBtn->setText(m_adhanEnabled ? "🔊" : "🔇");

    // Invisible spacer matching the button size — keeps time column aligned
    m_spacer = new QWidget;
    m_spacer->setFixedSize(28, 28);
    m_spacer->setVisible(false);

    layout->addWidget(m_nameLabel);
    layout->addStretch();
    layout->addWidget(m_timeLabel);
    layout->addSpacing(8);
    layout->addWidget(m_adhanBtn);
    layout->addWidget(m_spacer);
}

void PrayerWidget::setTime(const QString &time)
{
    m_time = time;
    m_timeLabel->setText(time);
}

void PrayerWidget::setAdhanEnabled(bool enabled)
{
    m_adhanEnabled = enabled;
    m_adhanBtn->blockSignals(true);
    m_adhanBtn->setChecked(enabled);
    m_adhanBtn->setText(enabled ? "🔊" : "🔇");
    m_adhanBtn->blockSignals(false);
}

void PrayerWidget::setAdhanVisible(bool visible)
{
    m_adhanVisible = visible;
    m_adhanBtn->setVisible(visible);
    m_spacer->setVisible(!visible);
}
