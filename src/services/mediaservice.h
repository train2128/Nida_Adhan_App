#ifndef MEDIASERVICE_H
#define MEDIASERVICE_H

#include <QObject>
#include <QStringList>

class MediaService : public QObject
{
    Q_OBJECT
public:
    explicit MediaService(QObject *parent = nullptr);

    void suspendMedia();
    void resumeMedia();
    // True while media that Nida paused is waiting to be resumed.
    bool pausedSomething() const { return m_suspended && m_pausedAny; }

private:
    bool m_wasMuted = false;
    int m_previousVolume = 100;
    bool m_suspended = false;
    bool m_pausedAny = false;
    QStringList m_pausedPlayers;
};

#endif
