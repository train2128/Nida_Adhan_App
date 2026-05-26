#ifndef WMMEDIACONTROL_H
#define WMMEDIACONTROL_H

#include <QObject>

class WmiMediaControl : public QObject
{
    Q_OBJECT
public:
    static void pauseAll();
    static void resumeAll();

private:
    static void sendMediaKey(DWORD key);
    static void muteSystem(bool mute);
    static bool s_muted;
    static int s_savedVolume;
};

#endif
