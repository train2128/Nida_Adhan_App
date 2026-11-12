#include "prayerwidget.h"
#include "theme.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

PrayerWidget::PrayerWidget(const QString &name, const QString &nameAr, QWidget *parent)
    : QWidget(parent)
    , m_name(name)
{
    setObjectName("prayerRow");
    setAttribute(Qt::WA_StyledBackground); // let QSS paint the row background

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 6, 6, 6);
    layout->setSpacing(6);

    m_nameLabel = new QLabel(name);
    m_nameLabel->setObjectName("prayerName");
    m_nameArLabel = new QLabel(nameAr);
    m_nameArLabel->setObjectName("prayerNameAr");

    m_timeLabel = new QLabel(QStringLiteral("--:--"));
    m_timeLabel->setObjectName("prayerTime");
    m_timeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_adhanBtn = new QPushButton;
    m_adhanBtn->setObjectName("adhanToggle");
    m_adhanBtn->setFixedSize(28, 28);
    m_adhanBtn->setIconSize(QSize(16, 16));
    m_adhanBtn->setCheckable(true);
    m_adhanBtn->setChecked(true);
    m_adhanBtn->setCursor(Qt::PointingHandCursor);
    m_adhanBtn->setToolTip(QStringLiteral("Play the adhan for %1").arg(name));
    connect(m_adhanBtn, &QPushButton::toggled, this, [this](bool checked) {
        m_adhanEnabled = checked;
        updateIcon();
        emit adhanToggled(m_name, checked);
    });

    layout->addWidget(m_nameLabel);
    layout->addWidget(m_nameArLabel);
    layout->addStretch();
    layout->addWidget(m_timeLabel);
    layout->addSpacing(6);
    layout->addWidget(m_adhanBtn);
    updateIcon();
}

void PrayerWidget::setTime(const QString &time)
{
    m_timeLabel->setText(time);
}

void PrayerWidget::setAdhanEnabled(bool enabled)
{
    m_adhanEnabled = enabled;
    m_adhanBtn->blockSignals(true);
    m_adhanBtn->setChecked(enabled);
    m_adhanBtn->blockSignals(false);
    updateIcon();
}

void PrayerWidget::setAdhanVisible(bool visible)
{
    m_adhanVisible = visible;
    // Sunrise keeps the same column but shows a static sun icon.
    m_adhanBtn->setAttribute(Qt::WA_TransparentForMouseEvents, !visible);
    m_adhanBtn->setFocusPolicy(visible ? Qt::StrongFocus : Qt::NoFocus);
    m_adhanBtn->setCursor(visible ? Qt::PointingHandCursor : Qt::ArrowCursor);
    m_adhanBtn->setToolTip(visible ? QStringLiteral("Play the adhan for %1").arg(m_name)
                                   : QStringLiteral("No adhan at sunrise"));
    updateIcon();
}

void PrayerWidget::setState(State state)
{
    if (state == m_state)
        return;
    m_state = state;
    const char *value = state == State::Next ? "next"
                      : state == State::Passed ? "passed" : "";
    setProperty("state", QString::fromLatin1(value));
    Theme::repolish(this);
    updateIcon();
}

void PrayerWidget::setDark(bool dark)
{
    m_dark = dark;
    updateIcon();
}

void PrayerWidget::updateIcon()
{
    const ThemeColors &c = Theme::colors(m_dark);
    QColor color = c.muted;
    if (m_state == State::Next && (m_adhanEnabled || !m_adhanVisible))
        color = c.accent;
    QString name;
    if (!m_adhanVisible)
        name = QStringLiteral("sun");
    else
        name = m_adhanEnabled ? QStringLiteral("bell") : QStringLiteral("bell-off");
    if (m_state == State::Passed || (m_adhanVisible && !m_adhanEnabled))
        color.setAlphaF(0.55);
    m_adhanBtn->setIcon(Theme::icon(name, color, 16));
}
