#ifndef NIDAAPP_H
#define NIDAAPP_H

#include <QObject>
#include <QStringList>
#include "models/prayertimes.h"

// Parsed "--test-adhan [--delay SECONDS] [--prayer NAME]" command line.
struct TestAdhanRequest {
    bool requested = false;
    int delaySec = 30;
    QString prayer;   // empty = the next upcoming prayer
    QString error;    // non-empty = invalid arguments
};
TestAdhanRequest parseTestAdhanArgs(const QStringList &args);

class QLocalServer;
class QTimer;
class SystemTray;
class MainWidget;
class SettingsDialog;
class StorageService;
class ApiService;
class MediaService;
class AdhanScheduler;
class AdhanNotificationWindow;

class NidaApp : public QObject
{
    Q_OBJECT
public:
    explicit NidaApp(QObject *parent = nullptr);
    ~NidaApp();

    void initialize();

private slots:
    void onShowWidget();
    void onSettingsRequested();
    void onAdhanStarted(const QString &prayerName);
    void onPrayerNotification(const QString &prayerName);
    void onAdhanDismissed();   // popup closed (audio keeps playing)
    void onStopAdhan();        // "Stop adhan" from popup or tray
    void onAdhanFinished();
    void applyTheme();
    void onPrayerTimesUpdated(const QString &name, int secondsUntil);
    void onPrayerTimesFetched(const DailyPrayerTimes &times);
    void onFetchError(const QString &error);
    void onSettingsChanged();
    void onDateChanged();
    void onRetryFetch();
    void onTestAdhanTimeout();

private:
    void refreshPrayerTimes();
    void applyPrayerTimes(const DailyPrayerTimes &times);
    void scheduleRetry();
    void scheduleTestAdhan(int delaySec, const QString &prayer);
    void setupAutoStart();
    void setupIpcServer();
    void handleCommandLine();
    void applyThemeToNotification();
    void showNotificationWindow(const QString &prayerName, bool withAdhan);
    QString launchPath() const;
    bool tryForwardToRunningInstance(const QString &message);

    SystemTray *m_tray = nullptr;
    MainWidget *m_widget = nullptr;
    SettingsDialog *m_settings = nullptr;
    StorageService *m_storage = nullptr;
    ApiService *m_api = nullptr;
    MediaService *m_media = nullptr;
    AdhanScheduler *m_scheduler = nullptr;
    AdhanNotificationWindow *m_notificationWindow = nullptr;
    QLocalServer *m_ipcServer = nullptr;
    QTimer *m_retryTimer = nullptr;
    int m_retryDelaySec = 0;
    QTimer *m_testTimer = nullptr;
    QString m_testPrayer;
    DailyPrayerTimes m_times;
};

#endif
