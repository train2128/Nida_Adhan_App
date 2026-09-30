#include "mainwidget.h"
#include "prayerwidget.h"
#include "storageservice.h"
#include "models/prayertimes.h"
#include <QFile>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMouseEvent>
#include <QApplication>
#include <QScreen>
#include <QWindow>
#include <QDebug>

MainWidget::MainWidget(StorageService *storage, QWidget *parent)
    : QWidget(parent)
    , m_storage(storage)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setupUi();
    applyTheme();

    // Auto-close when focus lost
    connect(qApp, &QApplication::focusWindowChanged, this, [this](QWindow *win) {
        if (!win && isVisible()) {
            hide();
        }
    });
}

void MainWidget::setupUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    auto *container = new QWidget(this);
    container->setObjectName("mainContainer");
    auto *layout = new QVBoxLayout(container);
    layout->setSpacing(4);
    layout->setContentsMargins(16, 12, 16, 12);

    // Date header
    auto *dateRow = new QHBoxLayout();
    m_hijriLabel = new QLabel;
    m_hijriLabel->setObjectName("hijriDate");
    m_miladiLabel = new QLabel;
    m_miladiLabel->setObjectName("miladiDate");
    m_miladiLabel->setAlignment(Qt::AlignRight);
    dateRow->addWidget(m_hijriLabel);
    dateRow->addStretch();
    dateRow->addWidget(m_miladiLabel);
    layout->addLayout(dateRow);

    // Separator
    auto *sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    sep->setObjectName("headerSep");
    layout->addWidget(sep);

    // Prayer rows
    m_fajrWidget = new PrayerWidget("Fajr", "الفجر", "04:12");
    m_sunriseWidget = new PrayerWidget("Sunrise", "الشروق", "05:40");
    m_dhuhrWidget = new PrayerWidget("Dhuhr", "الظهر", "12:23");
    m_asrWidget = new PrayerWidget("Asr", "العصر", "15:46");
    m_maghribWidget = new PrayerWidget("Maghrib", "المغرب", "19:06");
    m_ishaWidget = new PrayerWidget("Isha", "العشاء", "20:36");

    m_sunriseWidget->setAdhanVisible(false);

    layout->addWidget(m_fajrWidget);
    layout->addWidget(m_sunriseWidget);
    layout->addWidget(m_dhuhrWidget);
    layout->addWidget(m_asrWidget);
    layout->addWidget(m_maghribWidget);
    layout->addWidget(m_ishaWidget);

    // Connect adhan toggles
    auto connectToggle = [this](PrayerWidget *w) {
        connect(w, &PrayerWidget::adhanToggled, this, [this](const QString &name, bool on) {
            m_storage->setAdhanEnabled(name, on);
        });
    };
    connectToggle(m_fajrWidget);
    connectToggle(m_dhuhrWidget);
    connectToggle(m_asrWidget);
    connectToggle(m_maghribWidget);
    connectToggle(m_ishaWidget);

    // Separator
    auto *sep2 = new QFrame;
    sep2->setFrameShape(QFrame::HLine);
    sep2->setObjectName("footerSep");
    layout->addWidget(sep2);

    // Next prayer + settings row
    auto *footer = new QHBoxLayout();
    m_nextPrayerLabel = new QLabel;
    m_nextPrayerLabel->setObjectName("nextPrayer");
    m_settingsBtn = new QPushButton("⚙");
    m_settingsBtn->setObjectName("settingsBtn");
    m_settingsBtn->setFixedSize(32, 32);
    m_settingsBtn->setCursor(Qt::PointingHandCursor);
    footer->addWidget(m_nextPrayerLabel);
    footer->addStretch();
    footer->addWidget(m_settingsBtn);
    layout->addLayout(footer);

    root->addWidget(container);

    connect(m_settingsBtn, &QPushButton::clicked, this, &MainWidget::settingsRequested);
}

void MainWidget::applyTheme()
{
    NidaSettings s = m_storage->loadSettings();
    QString theme = s.darkTheme ? "nida_dark" : "nida_light";
    QFile f(QString(":/styles/%1").arg(theme));
    if (f.open(QFile::ReadOnly))
        setStyleSheet(f.readAll());
}

void MainWidget::setPrayerTimes(const DailyPrayerTimes &times)
{
    m_times = times;
    m_hijriLabel->setText(formatHijriDate(times.hijriDay, times.hijriMonthAr, times.hijriYear));
    m_miladiLabel->setText(times.miladiDate.toString("ddd d MMM yyyy"));

    m_fajrWidget->setTime(times.fajr.time.toString("HH:mm"));
    m_fajrWidget->setAdhanEnabled(m_storage->isAdhanEnabled("Fajr"));
    m_sunriseWidget->setTime(times.sunrise.time.toString("HH:mm"));
    m_dhuhrWidget->setTime(times.dhuhr.time.toString("HH:mm"));
    m_dhuhrWidget->setAdhanEnabled(m_storage->isAdhanEnabled("Dhuhr"));
    m_asrWidget->setTime(times.asr.time.toString("HH:mm"));
    m_asrWidget->setAdhanEnabled(m_storage->isAdhanEnabled("Asr"));
    m_maghribWidget->setTime(times.maghrib.time.toString("HH:mm"));
    m_maghribWidget->setAdhanEnabled(m_storage->isAdhanEnabled("Maghrib"));
    m_ishaWidget->setTime(times.isha.time.toString("HH:mm"));
    m_ishaWidget->setAdhanEnabled(m_storage->isAdhanEnabled("Isha"));
}

void MainWidget::setTimeUntilNext(const QString &name, int seconds)
{
    int h = seconds / 3600;
    int m = (seconds % 3600) / 60;
    m_nextPrayerLabel->setText(QString("Next: %1 in %2h %3m").arg(name).arg(h).arg(m));
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

void MainWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange || event->type() == QEvent::PaletteChange)
        applyTheme();
    QWidget::changeEvent(event);
}
