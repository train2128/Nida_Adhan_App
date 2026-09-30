#ifndef ADHANSCHEDULER_H
#define ADHANSCHEDULER_H

#include <QObject>
#include <QTimer>
#include <QMediaPlayer>
#include <QAudioOutput>
#include "models/prayertimes.h"

class StorageService;
class MediaService;

class AdhanScheduler : public QObject
{
    Q_OBJECT
public:
    explicit AdhanScheduler(StorageService *storage, MediaService *media,
                            QObject *parent = nullptr);

    void setPrayerTimes(const DailyPrayerTimes &times);
    void setVolume(int volume);
    void setAdhanSound(const QString &soundPath);
    void start();
    void stop();

    int secondsUntilNextPrayer() const;
    QString nextPrayerName() const;
    QTime nextPrayerTime() const;
    void triggerTestAdhan();

signals:
    void nextPrayerChanged(const QString &name, int secondsUntil);
    void adhanStarted(const QString &prayerName);
    void adhanFinished();
    void prayerNotification(const QString &prayerName);

public slots:
    void dismissAdhan();

private slots:
    void onTick();
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);

private:
    void playAdhan();
    void checkPrayerTimes();

    QTimer *m_timer;
    QMediaPlayer *m_player;
    QAudioOutput *m_audio;
    StorageService *m_storage;
    MediaService *m_media;
    DailyPrayerTimes m_times;
    QString m_adhanSound = "Adhan-Makkah";
    int m_volume = 80;
    int m_lastCheckedMinute = -1;
    bool m_adhanPlaying = false;
};

#endif
