#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include <QTimer>
#include "models/prayertimes.h"

class StorageService;
class PrayerWidget;
class QLabel;
class QVBoxLayout;
class QPushButton;

class MainWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MainWidget(StorageService *storage, QWidget *parent = nullptr);

    void setPrayerTimes(const DailyPrayerTimes &times);
    void setTimeUntilNext(const QString &name, int seconds);
    void showAdhanNotification(const QString &prayerName);

signals:
    void settingsRequested();
    void adhanDismissed();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    void setupUi();
    void applyTheme();
    void updateAdhanToggle(const QString &prayerName, bool enabled);

    StorageService *m_storage;
    DailyPrayerTimes m_times;

    QLabel *m_hijriLabel;
    QLabel *m_miladiLabel;
    PrayerWidget *m_fajrWidget;
    PrayerWidget *m_sunriseWidget;
    PrayerWidget *m_dhuhrWidget;
    PrayerWidget *m_asrWidget;
    PrayerWidget *m_maghribWidget;
    PrayerWidget *m_ishaWidget;
    QLabel *m_nextPrayerLabel;
    QPushButton *m_settingsBtn;
    QPoint m_dragPos;

    // Notification overlay
    QWidget *m_overlay;
    QLabel *m_overlayTitle;
    QLabel *m_overlayMsg;
    QPushButton *m_dismissBtn;
};

#endif
