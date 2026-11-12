#ifndef WMMEDIACONTROL_H
#define WMMEDIACONTROL_H

#include <QObject>
#include <windows.h>
class WmiMediaControl : public QObject
{
    Q_OBJECT
public:
    static bool pauseAll(); // true if another app was playing and got paused
    static void resumeAll();

private:
    static void sendMediaKey(DWORD key);
    // True when another process has an active audio session on the default
    // output device (i.e. something is actually playing).
    static bool otherAudioPlaying();
    static bool s_sentPause;
};

#endif
