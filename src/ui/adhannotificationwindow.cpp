#include "adhannotificationwindow.h"
#include "theme.h"
#include <QCursor>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QScreen>
#include <QVBoxLayout>

namespace {
QString arabicName(const QString &prayer)
{
    if (prayer == QLatin1String("Fajr")) return QStringLiteral("الفجر");
    if (prayer == QLatin1String("Dhuhr")) return QStringLiteral("الظهر");
    if (prayer == QLatin1String("Asr")) return QStringLiteral("العصر");
    if (prayer == QLatin1String("Maghrib")) return QStringLiteral("المغرب");
    if (prayer == QLatin1String("Isha")) return QStringLiteral("العشاء");
    if (prayer == QLatin1String("TEST")) return QStringLiteral("اختبار");
    return prayer;
}

QString mmss(qint64 ms)
{
    const qint64 s = qMax<qint64>(0, ms / 1000);
    return QStringLiteral("%1:%2").arg(s / 60).arg(s % 60, 2, 10, QLatin1Char('0'));
}
}

AdhanNotificationWindow::AdhanNotificationWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setObjectName("adhanNotificationWindow");
    setFixedWidth(340);
    setupUi();
    setDark(true);
}

void AdhanNotificationWindow::setupUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    auto *container = new QWidget;
    container->setObjectName("notificationContainer");
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(22, 22, 22, 18);
    layout->setSpacing(4);

    m_badge = new QLabel;
    m_badge->setObjectName("moonBadge");
    m_badge->setFixedSize(52, 52);
    m_badge->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_badge, 0, Qt::AlignHCenter);
    layout->addSpacing(8);

    m_nameAr = new QLabel;
    m_nameAr->setObjectName("notifNameAr");
    m_nameAr->setAlignment(Qt::AlignCenter);
    m_subtitle = new QLabel;
    m_subtitle->setObjectName("notifSubtitle");
    m_subtitle->setAlignment(Qt::AlignCenter);
    m_reminder = new QLabel(QStringLiteral("لا تنسَ صلاتك"));
    m_reminder->setObjectName("notifReminder");
    m_reminder->setAlignment(Qt::AlignCenter);
    m_reminderEn = new QLabel(QStringLiteral("Don't forget your prayer"));
    m_reminderEn->setObjectName("notifSmall");
    m_reminderEn->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_nameAr);
    layout->addWidget(m_subtitle);
    layout->addSpacing(10);
    layout->addWidget(m_reminder);
    layout->addWidget(m_reminderEn);
    layout->addSpacing(12);

    m_progressRow = new QWidget;
    auto *progressLayout = new QHBoxLayout(m_progressRow);
    progressLayout->setContentsMargins(0, 0, 0, 0);
    progressLayout->setSpacing(8);
    m_volumeIcon = new QLabel;
    m_progress = new QProgressBar;
    m_progress->setObjectName("adhanProgress");
    m_progress->setTextVisible(false);
    m_progress->setFixedHeight(4);
    m_progress->setRange(0, 1000);
    m_progressTime = new QLabel(QStringLiteral("0:00"));
    m_progressTime->setObjectName("notifSmall");
    progressLayout->addWidget(m_volumeIcon);
    progressLayout->addWidget(m_progress, 1);
    progressLayout->addWidget(m_progressTime);
    layout->addWidget(m_progressRow);
    layout->addSpacing(14);

    auto *btnRow = new QHBoxLayout;
    btnRow->setSpacing(8);
    m_stopBtn = new QPushButton(QStringLiteral("Stop adhan"));
    m_stopBtn->setObjectName("primaryBtn");
    m_closeBtn = new QPushButton(QStringLiteral("Close"));
    m_closeBtn->setObjectName("secondaryBtn");
    for (QPushButton *b : {m_stopBtn, m_closeBtn}) {
        b->setCursor(Qt::PointingHandCursor);
        b->setIconSize(QSize(15, 15));
    }
    btnRow->addStretch();
    btnRow->addWidget(m_stopBtn);
    btnRow->addWidget(m_closeBtn);
    btnRow->addStretch();
    layout->addLayout(btnRow);

    m_note = new QLabel;
    m_note->setObjectName("notifSmall");
    m_note->setAlignment(Qt::AlignCenter);
    m_note->setWordWrap(true);
    layout->addSpacing(8);
    layout->addWidget(m_note);

    root->addWidget(container);

    connect(m_stopBtn, &QPushButton::clicked, this, &AdhanNotificationWindow::stopRequested);
    connect(m_closeBtn, &QPushButton::clicked, this, [this]() {
        hide();
        emit closed();
    });
}

void AdhanNotificationWindow::setDark(bool dark)
{
    m_dark = dark;
    Theme::apply(this, dark);
    const ThemeColors &c = Theme::colors(dark);
    m_badge->setPixmap(Theme::pixmap(QStringLiteral("moon"), c.gold, 24));
    m_volumeIcon->setPixmap(Theme::pixmap(QStringLiteral("volume"), c.muted, 15));
    m_stopBtn->setIcon(Theme::icon(QStringLiteral("stop"), c.onAccent, 15));
    m_closeBtn->setIcon(Theme::icon(QStringLiteral("x"), c.text, 15));
}

void AdhanNotificationWindow::showForPrayer(const QString &prayerName, const QTime &prayerTime,
                                            bool withAdhan, bool mediaPaused)
{
    m_mediaPaused = mediaPaused;
    const bool test = prayerName == QLatin1String("TEST");
    const QString time = (prayerTime.isValid() ? prayerTime : QTime::currentTime()).toString("HH:mm");
    m_nameAr->setText(arabicName(prayerName));
    m_subtitle->setText(test ? QStringLiteral("Test adhan · %1").arg(time)
                             : QStringLiteral("It's time for %1 · %2").arg(prayerName, time));

    m_progress->setValue(0);
    m_progressTime->setText(QStringLiteral("0:00"));
    m_progressRow->setVisible(withAdhan);
    m_stopBtn->setVisible(withAdhan);
    m_closeBtn->setText(withAdhan ? QStringLiteral("Close") : QStringLiteral("OK"));
    if (!withAdhan)
        m_note->setText(QStringLiteral("The adhan sound is off for this prayer"));
    else if (mediaPaused)
        m_note->setText(QStringLiteral("Your media was paused and resumes after the adhan"));
    else
        m_note->clear();
    m_note->setVisible(!m_note->text().isEmpty());

    adjustSize();
    QScreen *screen = QGuiApplication::screenAt(QCursor::pos());
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    if (screen) {
        const QRect geo = screen->availableGeometry();
        move(geo.center().x() - width() / 2, geo.center().y() - height() / 2);
    }
    show();
    raise();
    activateWindow();
}

void AdhanNotificationWindow::setProgress(qint64 positionMs, qint64 durationMs)
{
    if (durationMs > 0) {
        m_progress->setValue(int(qBound(qint64(0), 1000 * positionMs / durationMs, qint64(1000))));
        m_progressTime->setText(mmss(positionMs) + QStringLiteral(" / ") + mmss(durationMs));
    } else {
        m_progressTime->setText(mmss(positionMs));
    }
}

void AdhanNotificationWindow::setAdhanFinished()
{
    m_stopBtn->setVisible(false);
    m_progressRow->setVisible(false);
    m_closeBtn->setText(QStringLiteral("Close"));
    m_note->setText(m_mediaPaused ? QStringLiteral("Adhan finished · your media has resumed")
                                  : QStringLiteral("Adhan finished"));
    m_note->setVisible(true);
    adjustSize();
}
