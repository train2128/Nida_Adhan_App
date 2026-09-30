#ifndef NIDAAPP_H
#define NIDAAPP_H

#include <QObject>
#include "models/prayertimes.h"

class QLocalServer;
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
    void onAdhanDismissed();
    void onPrayerTimesUpdated(const QString &name, int secondsUntil);
    void onPrayerTimesFetched(const DailyPrayerTimes &times);
    void onFetchError(const QString &error);
    void onSettingsChanged();

private:
    void refreshPrayerTimes();
    void setupAutoStart();
    void setupIpcServer();
    void handleCommandLine();
    void applyThemeToNotification();
    void showNotificationWindow(const QString &prayerName);
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
    DailyPrayerTimes m_times;
};

#endif
