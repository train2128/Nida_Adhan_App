#include "adhanscheduler.h"
#include "storageservice.h"
#include "mediaservice.h"
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QUrl>

namespace {
bool isAdhanResourceOrUrl(const QString &p)
{
    return p.startsWith(QStringLiteral("qrc:/"))
        || p.startsWith(QStringLiteral("qrc:"))
        || p.startsWith(QStringLiteral(":/"))
        || p.startsWith(QStringLiteral("file://"));
}

QUrl resolveAdhanSource(const QString &sound)
{
    QString path = sound;
    // Built-in name (e.g. "Adhan-Makkah") -> bundled resource.
    // QDir::isAbsolutePath handles Unix "/", Windows "C:/", "C:\", UNC "\\".
    if (!isAdhanResourceOrUrl(path) && !QDir::isAbsolutePath(path))
        path = QStringLiteral("qrc:/sounds/%1").arg(path);

    if (isAdhanResourceOrUrl(path))
        return QUrl(path);
    return QUrl::fromLocalFile(path);
}

const QString kFallbackSound = QStringLiteral("Adhan-Makkah");
}

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
    connect(m_player, &QMediaPlayer::errorOccurred,
            this, &AdhanScheduler::onPlayerError);
    connect(m_timer, &QTimer::timeout, this, &AdhanScheduler::onTick);
    connect(m_player, &QMediaPlayer::positionChanged, this, [this](qint64 pos) {
        if (m_adhanPlaying)
            emit playbackProgress(pos, m_player->duration());
    });
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
    // Force a check now; m_firedPrayers prevents replaying a prayer that
    // was already announced during this minute.
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

QTime AdhanScheduler::prayerTime(const QString &prayerName) const
{
    const PrayerTimeEntry *entries[] = {&m_times.fajr, &m_times.sunrise, &m_times.dhuhr,
                                        &m_times.asr, &m_times.maghrib, &m_times.isha};
    for (const auto *e : entries) {
        if (e->name == prayerName)
            return e->time;
    }
    return {};
}

bool AdhanScheduler::mediaPaused() const
{
    return m_media->pausedSomething();
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
        finishPlayback();
    }
}

void AdhanScheduler::finishPlayback()
{
    if (!m_adhanPlaying)
        return;
    m_adhanPlaying = false;
    m_media->resumeMedia();
    emit adhanFinished();
}

void AdhanScheduler::onTick()
{
    const QDateTime nowDt = QDateTime::currentDateTime();
    const QDate today = nowDt.date();
    if (m_lastTickDate.isValid() && today != m_lastTickDate) {
        m_firedPrayers.clear();
        m_lastTickDate = today;
        emit dateChanged();
    }
    m_lastTickDate = today;

    const QTime nowTime = nowDt.time();
    int currentMinute = nowTime.hour() * 60 + nowTime.minute();
    if (currentMinute == m_lastCheckedMinute) return;
    m_lastCheckedMinute = currentMinute;
    checkPrayerTimes();
    emit nextPrayerChanged(nextPrayerName(), secondsUntilNextPrayer());
}

void AdhanScheduler::checkPrayerTimes()
{
    const QDateTime nowDt = QDateTime::currentDateTime();
    const QTime now = nowDt.time();
    const QString dayKey = nowDt.date().toString(Qt::ISODate);
    auto check = [&](const PrayerTimeEntry &entry) {
        if (entry.time.isNull() || !entry.time.isValid())
            return;
        if (!m_adhanPlaying && now.hour() == entry.time.hour() && now.minute() == entry.time.minute()) {
            const QString key = dayKey + QLatin1Char('/') + entry.name;
            if (m_firedPrayers.contains(key))
                return; // already announced (e.g. user dismissed it)
            m_firedPrayers.insert(key);
            announcePrayer(entry.name);
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

    // A custom file may have been moved/deleted since it was chosen: fall
    // back to a bundled adhan instead of playing nothing.
    QString sound = m_adhanSound;
    const QUrl requested = resolveAdhanSource(sound);
    if (requested.isLocalFile() && !QFileInfo::exists(requested.toLocalFile())) {
        qWarning() << "Adhan file missing, using built-in:" << requested.toLocalFile();
        sound = kFallbackSound;
    }
    m_usingFallbackSound = (sound == kFallbackSound);

    const QUrl source = resolveAdhanSource(sound);

    m_player->setSource(source);
    m_audio->setVolume(m_volume / 100.0f);
    m_player->play();
    m_adhanPlaying = true;
}

void AdhanScheduler::announcePrayer(const QString &prayerName)
{
    if (m_storage->isAdhanEnabled(prayerName)) {
        playAdhan();
        emit adhanStarted(prayerName);
    } else {
        emit prayerNotification(prayerName);
    }
}

void AdhanScheduler::triggerTestAdhan()
{
    playAdhan();
    emit adhanStarted("TEST");
}

void AdhanScheduler::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia && m_adhanPlaying) {
        // Audio finished on its own: resume what we paused and notify UI.
        finishPlayback();
    }
}

void AdhanScheduler::onPlayerError(QMediaPlayer::Error error, const QString &errorString)
{
    if (error == QMediaPlayer::NoError || !m_adhanPlaying)
        return;
    qWarning() << "Adhan playback failed:" << errorString;
    if (!m_usingFallbackSound) {
        // Custom/selected sound is unplayable: try the bundled one once.
        m_usingFallbackSound = true;
        m_player->setSource(resolveAdhanSource(kFallbackSound));
        m_player->play();
        return;
    }
    // Even the fallback failed: don't stay stuck in "playing" (which would
    // block every later adhan and leave other media paused).
    m_player->stop();
    finishPlayback();
}
