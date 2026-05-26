#ifndef NIDAAPP_H
#define NIDAAPP_H

#include <QObject>
#include "models/prayertimes.h"

class SystemTray;
class MainWidget;
class SettingsDialog;
class StorageService;
class ApiService;
class MediaService;
class AdhanScheduler;

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
    void onAdhanDismissed();
    void onPrayerTimesUpdated(const QString &name, int secondsUntil);
    void onPrayerTimesFetched(const DailyPrayerTimes &times);
    void onFetchError(const QString &error);
    void onSettingsChanged();

private:
    void refreshPrayerTimes();
    void setupAutoStart();

    SystemTray *m_tray;
    MainWidget *m_widget;
    SettingsDialog *m_settings;
    StorageService *m_storage;
    ApiService *m_api;
    MediaService *m_media;
    AdhanScheduler *m_scheduler;
    DailyPrayerTimes m_times;
};

#endif
