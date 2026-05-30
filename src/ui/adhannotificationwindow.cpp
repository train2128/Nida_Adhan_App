#include "adhannotificationwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScreen>
#include <QGuiApplication>
#include <QDateTime>

AdhanNotificationWindow::AdhanNotificationWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setObjectName("adhanNotificationWindow");
    setFixedSize(340, 220);
    setupUi();
}

void AdhanNotificationWindow::setupUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    auto *container = new QWidget;
    container->setObjectName("notificationContainer");
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(10);

    m_prayerNameLabel = new QLabel;
    m_prayerNameLabel->setObjectName("notificationPrayerName");
    m_prayerNameLabel->setAlignment(Qt::AlignCenter);

    m_reminderLabel = new QLabel(QString::fromUtf8("\xD9\x84\xD8\xA7 \xD8\xAA\xD9\x86\xD8\xB3\xD9\x89 \xD8\xB5\xD9\x84\xD8\xA7\xD8\xAA\xD9\x83"));
    m_reminderLabel->setObjectName("notificationReminder");
    m_reminderLabel->setAlignment(Qt::AlignCenter);

    m_timeLabel = new QLabel;
    m_timeLabel->setObjectName("notificationTime");
    m_timeLabel->setAlignment(Qt::AlignCenter);

    auto *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    m_closeBtn = new QPushButton(QString::fromUtf8("\xD8\xA3\xD8\xBA\xD9\x84\xD9\x82 / Close"));
    m_closeBtn->setObjectName("notificationCloseBtn");
    m_closeBtn->setCursor(Qt::PointingHandCursor);
    btnRow->addWidget(m_closeBtn);
    btnRow->addStretch();

    layout->addStretch();
    layout->addWidget(m_prayerNameLabel);
    layout->addWidget(m_reminderLabel);
    layout->addWidget(m_timeLabel);
    layout->addStretch();
    layout->addLayout(btnRow);

    root->addWidget(container);

    connect(m_closeBtn, &QPushButton::clicked, this, [this]() {
        emit dismissed();
        hide();
    });
}

void AdhanNotificationWindow::showForPrayer(const QString &prayerName)
{
    if (prayerName == "TEST") {
        m_prayerNameLabel->setText("TEST");
    } else {
        QString ar;
        if (prayerName == "Fajr") ar = QString::fromUtf8("\xD8\xA7\xD9\x84\xD9\x81\xD8\xAC\xD8\xB1");
        else if (prayerName == "Dhuhr") ar = QString::fromUtf8("\xD8\xA7\xD9\x84\xD8\xB8\xD9\x87\xD8\xB1");
        else if (prayerName == "Asr") ar = QString::fromUtf8("\xD8\xA7\xD9\x84\xD8\xB9\xD8\xB5\xD8\xB1");
        else if (prayerName == "Maghrib") ar = QString::fromUtf8("\xD8\xA7\xD9\x84\xD9\x85\xD8\xBA\xD8\xB1\xD8\xA8");
        else if (prayerName == "Isha") ar = QString::fromUtf8("\xD8\xA7\xD9\x84\xD8\xB9\xD8\xB4\xD8\xA7\xD8\xA1");
        else ar = prayerName;
        m_prayerNameLabel->setText(prayerName + " - " + ar);
    }

    m_timeLabel->setText(QDateTime::currentDateTime().toString("hh:mm"));

    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect geo = screen->availableGeometry();
        move(geo.center().x() - width() / 2, geo.center().y() - height() / 2);
    }

    show();
    raise();
    activateWindow();
}
