#include "mediaservice.h"
#include <QDebug>

#if defined(NIDA_OS_LINUX)
#include "mpriscontroller.h"
#elif defined(NIDA_OS_WINDOWS)
#include "wmmediacontrol.h"
#endif

MediaService::MediaService(QObject *parent)
    : QObject(parent)
{
}

void MediaService::suspendMedia()
{
    if (m_suspended) return;
    m_suspended = true;

#if defined(NIDA_OS_LINUX)
    m_pausedPlayers = MprisController::pauseAll();
    m_pausedAny = !m_pausedPlayers.isEmpty();
#elif defined(NIDA_OS_WINDOWS)
    m_pausedAny = WmiMediaControl::pauseAll();
#else
    qWarning() << "MediaService::suspendMedia: unsupported platform, no-op";
#endif

    qDebug() << "Media suspended";
}

void MediaService::resumeMedia()
{
    if (!m_suspended) return;
    m_suspended = false;
    m_pausedAny = false;

#if defined(NIDA_OS_LINUX)
    MprisController::resumePlayers(m_pausedPlayers);
    m_pausedPlayers.clear();
#elif defined(NIDA_OS_WINDOWS)
    WmiMediaControl::resumeAll();
#else
    qWarning() << "MediaService::resumeMedia: unsupported platform, no-op";
#endif

    qDebug() << "Media resumed";
}
