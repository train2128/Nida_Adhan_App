#ifndef ADHANSCHEDULER_H
#define ADHANSCHEDULER_H

#include <QObject>
#include <QTimer>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QDate>
#include <QSet>
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
    QTime prayerTime(const QString &prayerName) const;
    bool isAdhanPlaying() const { return m_adhanPlaying; }
    bool mediaPaused() const;
    void triggerTestAdhan();
    // Runs exactly what happens when a prayer time is reached (respects the
    // per-prayer adhan toggle, pauses/resumes media). Used by --test-adhan.
    void announcePrayer(const QString &prayerName);

signals:
    void nextPrayerChanged(const QString &name, int secondsUntil);
    void adhanStarted(const QString &prayerName);
    void adhanFinished();
    void prayerNotification(const QString &prayerName);
    // Emitted when the calendar date changes (midnight, or after the system
    // wakes up on a later day) so the owner can fetch the new day's times.
    void dateChanged();
    void playbackProgress(qint64 positionMs, qint64 durationMs);

public slots:
    void dismissAdhan();

private slots:
    void onTick();
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onPlayerError(QMediaPlayer::Error error, const QString &errorString);

private:
    void playAdhan();
    void checkPrayerTimes();
    void finishPlayback();

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
    bool m_usingFallbackSound = false;
    QDate m_lastTickDate;
    // "yyyy-MM-dd/Name" of prayers already announced, so a scheduler restart
    // (settings save, fresh fetch) inside the prayer minute does not replay.
    QSet<QString> m_firedPrayers;
};

#endif
