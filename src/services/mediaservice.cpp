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
    MprisController::pauseAll();
#elif defined(NIDA_OS_WINDOWS)
    WmiMediaControl::pauseAll();
#endif

    qDebug() << "Media suspended";
}

void MediaService::resumeMedia()
{
    if (!m_suspended) return;
    m_suspended = false;

#if defined(NIDA_OS_LINUX)
    MprisController::resumeAll();
#elif defined(NIDA_OS_WINDOWS)
    WmiMediaControl::resumeAll();
#endif

    qDebug() << "Media resumed";
}
