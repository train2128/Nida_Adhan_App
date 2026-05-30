#include "adhanscheduler.h"
#include "storageservice.h"
#include "mediaservice.h"
#include <QDateTime>
#include <QDebug>
#include <QUrl>

AdhanScheduler::AdhanScheduler(StorageService *storage, MediaService *media,
                               QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
    , m_player(new QMediaPlayer(this))
    , m_audio(new QAudioOutput(this))
    , m_storage(storage)
    , m_media(media)
{
    m_player->setAudioOutput(m_audio);
    connect(m_player, &QMediaPlayer::mediaStatusChanged,
            this, &AdhanScheduler::onMediaStatusChanged);
    connect(m_timer, &QTimer::timeout, this, &AdhanScheduler::onTick);
}

void AdhanScheduler::setPrayerTimes(const DailyPrayerTimes &times)
{
    m_times = times;
}

void AdhanScheduler::setVolume(int volume)
{
    m_volume = qBound(0, volume, 100);
    m_audio->setVolume(m_volume / 100.0f);
}

void AdhanScheduler::setAdhanSound(const QString &soundPath)
{
    m_adhanSound = soundPath;
}

void AdhanScheduler::start()
{
    m_timer->start(30000);
    m_lastCheckedMinute = -1;
    onTick();
}

void AdhanScheduler::stop()
{
    m_timer->stop();
    if (m_adhanPlaying) {
        m_player->stop();
        m_adhanPlaying = false;
    }
}

int AdhanScheduler::secondsUntilNextPrayer() const
{
    return m_times.secondsUntilNext();
}

QString AdhanScheduler::nextPrayerName() const
{
    auto *np = m_times.nextPrayer();
    return np ? np->name : QString();
}

QTime AdhanScheduler::nextPrayerTime() const
{
    auto *np = m_times.nextPrayer();
    return np ? np->time : QTime();
}

void AdhanScheduler::dismissAdhan()
{
    if (m_adhanPlaying) {
        m_player->stop();
        m_adhanPlaying = false;
        m_media->resumeMedia();
        emit adhanFinished();
    }
}

void AdhanScheduler::onTick()
{
    int currentMinute = QTime::currentTime().hour() * 60 + QTime::currentTime().minute();
    if (currentMinute == m_lastCheckedMinute) return;
    m_lastCheckedMinute = currentMinute;
    checkPrayerTimes();
    emit nextPrayerChanged(nextPrayerName(), secondsUntilNextPrayer());
}

void AdhanScheduler::checkPrayerTimes()
{
    QTime now = QTime::currentTime();
    auto check = [&](const PrayerTimeEntry &entry) {
        if (!m_adhanPlaying && now.hour() == entry.time.hour() && now.minute() == entry.time.minute()) {
            if (m_storage->isAdhanEnabled(entry.name)) {
                playAdhan();
                emit adhanStarted(entry.name);
            } else {
                emit prayerNotification(entry.name);
            }
        }
    };
    check(m_times.fajr);
    check(m_times.dhuhr);
    check(m_times.asr);
    check(m_times.maghrib);
    check(m_times.isha);
}

void AdhanScheduler::playAdhan()
{
    m_media->suspendMedia();

    QString path = m_adhanSound;
    if (!m_adhanSound.startsWith("qrc:/") && !m_adhanSound.startsWith("/") && !m_adhanSound.startsWith("file://"))
        path = QString("qrc:/sounds/%1").arg(m_adhanSound);

    m_player->setSource(QUrl(path));
    m_audio->setVolume(m_volume / 100.0f);
    m_player->play();
    m_adhanPlaying = true;
}

void AdhanScheduler::triggerTestAdhan()
{
    playAdhan();
    emit adhanStarted("TEST");
}

void AdhanScheduler::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia && m_adhanPlaying) {
        m_adhanPlaying = false;
    }
}
