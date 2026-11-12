#include "mainwidget.h"
#include "prayerwidget.h"
#include "storageservice.h"
#include "theme.h"
#include <QApplication>
#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

namespace {
QString formatCountdown(qint64 secs)
{
    if (secs < 0)
        secs = 0;
    return QStringLiteral("%1:%2:%3")
        .arg(secs / 3600)
        .arg((secs % 3600) / 60, 2, 10, QLatin1Char('0'))
        .arg(secs % 60, 2, 10, QLatin1Char('0'));
}
}

MainWidget::MainWidget(StorageService *storage, QWidget *parent)
    : QWidget(parent)
    , m_storage(storage)
    , m_tick(new QTimer(this))
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFixedWidth(304);
    setupUi();
    applyTheme();

    m_tick->setInterval(1000);
    connect(m_tick, &QTimer::timeout, this, &MainWidget::updateCountdown);

    // Auto-close when focus lost
    connect(qApp, &QApplication::focusWindowChanged, this, [this](QWindow *win) {
        if (!win && isVisible())
            hide();
    });
}

void MainWidget::setupUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    auto *container = new QWidget(this);
    container->setObjectName("mainContainer");
    auto *layout = new QVBoxLayout(container);
    layout->setSpacing(2);
    layout->setContentsMargins(14, 14, 14, 12);

    // Header: Hijri + Gregorian date, location on the right.
    auto *header = new QHBoxLayout;
    auto *dates = new QVBoxLayout;
    dates->setSpacing(1);
    m_hijriLabel = new QLabel;
    m_hijriLabel->setObjectName("hijriDate");
    m_miladiLabel = new QLabel;
    m_miladiLabel->setObjectName("miladiDate");
    dates->addWidget(m_hijriLabel);
    dates->addWidget(m_miladiLabel);
    header->addLayout(dates);
    header->addStretch();
    m_locationIcon = new QLabel;
    m_locationLabel = new QLabel;
    m_locationLabel->setObjectName("locationLabel");
    header->addWidget(m_locationIcon, 0, Qt::AlignTop);
    header->addWidget(m_locationLabel, 0, Qt::AlignTop);
    layout->addLayout(header);
    layout->addSpacing(10);

    // Next-prayer card.
    auto *card = new QWidget;
    card->setObjectName("nextCard");
    card->setAttribute(Qt::WA_StyledBackground);
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(12, 10, 12, 12);
    cardLayout->setSpacing(2);
    auto *cardTop = new QHBoxLayout;
    m_nextCaption = new QLabel(QStringLiteral("Next prayer"));
    m_nextCaption->setObjectName("cardCaption");
    m_nextTimeLabel = new QLabel;
    m_nextTimeLabel->setObjectName("cardCaption");
    cardTop->addWidget(m_nextCaption);
    cardTop->addStretch();
    cardTop->addWidget(m_nextTimeLabel);
    auto *cardMid = new QHBoxLayout;
    cardMid->setSpacing(6);
    m_nextName = new QLabel(QStringLiteral("Loading..."));
    m_nextName->setObjectName("nextName");
    m_nextNameAr = new QLabel;
    m_nextNameAr->setObjectName("nextNameAr");
    m_countdown = new QLabel(QStringLiteral("--:--"));
    m_countdown->setObjectName("countdown");
    cardMid->addWidget(m_nextName, 0, Qt::AlignBaseline);
    cardMid->addWidget(m_nextNameAr, 0, Qt::AlignBaseline);
    cardMid->addStretch();
    cardMid->addWidget(m_countdown, 0, Qt::AlignBaseline);
    m_progress = new QProgressBar;
    m_progress->setObjectName("nextProgress");
    m_progress->setRange(0, 1000);
    m_progress->setValue(0);
    m_progress->setTextVisible(false);
    m_progress->setFixedHeight(4);
    cardLayout->addLayout(cardTop);
    cardLayout->addLayout(cardMid);
    cardLayout->addSpacing(8);
    cardLayout->addWidget(m_progress);
    layout->addWidget(card);
    layout->addSpacing(8);

    // Prayer rows.
    const QList<QPair<QString, QString>> names = {
        {"Fajr", "الفجر"}, {"Sunrise", "الشروق"}, {"Dhuhr", "الظهر"},
        {"Asr", "العصر"}, {"Maghrib", "المغرب"}, {"Isha", "العشاء"},
    };
    for (const auto &n : names) {
        auto *row = new PrayerWidget(n.first, n.second);
        if (n.first == QLatin1String("Sunrise"))
            row->setAdhanVisible(false);
        else
            row->setAdhanEnabled(m_storage->isAdhanEnabled(n.first));
        connect(row, &PrayerWidget::adhanToggled, this, [this](const QString &name, bool on) {
            m_storage->setAdhanEnabled(name, on);
        });
        m_rows.append(row);
        layout->addWidget(row);
    }

    // Footer: method + refresh + settings.
    auto *sep = new QFrame;
    sep->setObjectName("footerSep");
    sep->setFrameShape(QFrame::HLine);
    layout->addSpacing(6);
    layout->addWidget(sep);
    layout->addSpacing(6);
    auto *footer = new QHBoxLayout;
    m_methodLabel = new QLabel;
    m_methodLabel->setObjectName("methodLabel");
    m_refreshBtn = new QPushButton;
    m_refreshBtn->setObjectName("iconBtn");
    m_refreshBtn->setToolTip(QStringLiteral("Refresh prayer times"));
    m_settingsBtn = new QPushButton;
    m_settingsBtn->setObjectName("iconBtn");
    m_settingsBtn->setToolTip(QStringLiteral("Settings"));
    for (QPushButton *b : {m_refreshBtn, m_settingsBtn}) {
        b->setFixedSize(32, 32);
        b->setIconSize(QSize(17, 17));
        b->setCursor(Qt::PointingHandCursor);
    }
    footer->addWidget(m_methodLabel);
    footer->addStretch();
    footer->addWidget(m_refreshBtn);
    footer->addWidget(m_settingsBtn);
    layout->addLayout(footer);

    root->addWidget(container);

    connect(m_settingsBtn, &QPushButton::clicked, this, &MainWidget::settingsRequested);
    connect(m_refreshBtn, &QPushButton::clicked, this, &MainWidget::refreshRequested);
}

void MainWidget::applyTheme()
{
    const NidaSettings s = m_storage->loadSettings();
    m_dark = Theme::isDark(s);
    Theme::apply(this, m_dark);
    const ThemeColors &c = Theme::colors(m_dark);
    for (PrayerWidget *row : m_rows)
        row->setDark(m_dark);
    m_locationIcon->setPixmap(Theme::pixmap(QStringLiteral("map-pin"), c.muted, 13));
    m_refreshBtn->setIcon(Theme::icon(QStringLiteral("refresh"), c.text, 17));
    m_settingsBtn->setIcon(Theme::icon(QStringLiteral("settings"), c.text, 17));
    updateFooter();
}

void MainWidget::updateFooter()
{
    const NidaSettings s = m_storage->loadSettings();
    const QString city = !s.city.isEmpty() ? s.city : s.displayLocation();
    m_locationLabel->setText(city);
    m_locationLabel->setToolTip(s.displayLocation());
    m_methodLabel->setText(calculationMethodName(s.method));
}

void MainWidget::setPrayerTimes(const DailyPrayerTimes &times)
{
    m_times = times;
    m_hasTimes = times.isValid();
    m_hijriLabel->setText(times.hijriDay.isEmpty()
        ? QString()
        : formatHijriDate(times.hijriDay, times.hijriMonthAr, times.hijriYear));
    m_miladiLabel->setText((times.miladiDate.isValid() ? times.miladiDate
                                                       : QDate::currentDate())
                               .toString("ddd d MMM yyyy"));

    const QTime entries[] = {times.fajr.time, times.sunrise.time, times.dhuhr.time,
                             times.asr.time, times.maghrib.time, times.isha.time};
    for (int i = 0; i < m_rows.size(); ++i) {
        m_rows[i]->setTime(entries[i].isValid() ? entries[i].toString("HH:mm")
                                                : QStringLiteral("--:--"));
        if (m_rows[i]->name() != QLatin1String("Sunrise"))
            m_rows[i]->setAdhanEnabled(m_storage->isAdhanEnabled(m_rows[i]->name()));
    }
    updateFooter();
    updateCountdown();
}

void MainWidget::setTimeUntilNext(const QString &, int)
{
    // The card counts down every second on its own while visible; this keeps
    // it fresh when the minute ticks while hidden.
    updateCountdown();
}

void MainWidget::updateCountdown()
{
    if (!m_hasTimes) {
        m_nextName->setText(QStringLiteral("Loading prayer times..."));
        m_nextNameAr->clear();
        m_nextTimeLabel->clear();
        m_countdown->setText(QStringLiteral("--:--"));
        m_progress->setValue(0);
        return;
    }

    const QDateTime now = QDateTime::currentDateTime();
    const QDate today = now.date();
    const PrayerTimeEntry *entries[] = {&m_times.fajr, &m_times.sunrise, &m_times.dhuhr,
                                        &m_times.asr, &m_times.maghrib, &m_times.isha};
    int nextIdx = -1;
    for (int i = 0; i < 6; ++i) {
        if (QDateTime(today, entries[i]->time) > now) {
            nextIdx = i;
            break;
        }
    }

    QDateTime nextAt, prevAt;
    const PrayerTimeEntry *next;
    if (nextIdx >= 0) {
        next = entries[nextIdx];
        nextAt = QDateTime(today, next->time);
        prevAt = nextIdx > 0 ? QDateTime(today, entries[nextIdx - 1]->time)
                             : QDateTime(today.addDays(-1), m_times.isha.time);
    } else {
        // After Isha: tomorrow's Fajr (approximated with today's time).
        next = &m_times.fajr;
        nextAt = QDateTime(today.addDays(1), m_times.fajr.time);
        prevAt = QDateTime(today, m_times.isha.time);
    }

    for (int i = 0; i < m_rows.size(); ++i) {
        PrayerWidget::State st = PrayerWidget::State::Upcoming;
        if (i == (nextIdx >= 0 ? nextIdx : 0))
            st = PrayerWidget::State::Next;
        else if (nextIdx < 0 || i < nextIdx)
            st = PrayerWidget::State::Passed;
        m_rows[i]->setState(st);
    }

    m_nextName->setText(next->name);
    m_nextNameAr->setText(next->nameAr);
    m_nextTimeLabel->setText(next->time.toString("HH:mm"));
    const qint64 remaining = now.secsTo(nextAt);
    m_countdown->setText(formatCountdown(remaining));
    const qint64 span = prevAt.secsTo(nextAt);
    m_progress->setValue(span > 0 ? int(1000 * (span - remaining) / span) : 0);
}

void MainWidget::showEvent(QShowEvent *event)
{
    updateCountdown();
    m_tick->start();
    QWidget::showEvent(event);
}

void MainWidget::hideEvent(QHideEvent *event)
{
    m_tick->stop();
    QWidget::hideEvent(event);
}

void MainWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_dragPos = event->globalPosition().toPoint() - frameGeometry().topLeft();
}

void MainWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton)
        move(event->globalPosition().toPoint() - m_dragPos);
}
