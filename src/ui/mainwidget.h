#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include "models/prayertimes.h"

class StorageService;
class PrayerWidget;
class QLabel;
class QPushButton;
class QProgressBar;
class QTimer;

// Tray popup: date header, next-prayer card with live countdown, the six
// prayer rows (passed ones dimmed, next one highlighted) and a footer.
class MainWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MainWidget(StorageService *storage, QWidget *parent = nullptr);

    void setPrayerTimes(const DailyPrayerTimes &times);
    void setTimeUntilNext(const QString &name, int seconds); // kept for callers; countdown is live
    void applyTheme();        // re-read theme + location/method from settings

signals:
    void settingsRequested();
    void refreshRequested();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    void setupUi();
    void updateCountdown();
    void updateFooter();

    StorageService *m_storage;
    DailyPrayerTimes m_times;
    bool m_hasTimes = false;
    bool m_dark = true;

    QLabel *m_hijriLabel;
    QLabel *m_miladiLabel;
    QLabel *m_locationIcon;
    QLabel *m_locationLabel;

    QLabel *m_nextCaption;
    QLabel *m_nextTimeLabel;
    QLabel *m_nextName;
    QLabel *m_nextNameAr;
    QLabel *m_countdown;
    QProgressBar *m_progress;

    QList<PrayerWidget *> m_rows; // Fajr, Sunrise, Dhuhr, Asr, Maghrib, Isha

    QLabel *m_methodLabel;
    QPushButton *m_refreshBtn;
    QPushButton *m_settingsBtn;

    QTimer *m_tick;
    QPoint m_dragPos;
};

#endif
