#include "wmmediacontrol.h"
#include <QDebug>
#include <windows.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>

bool WmiMediaControl::s_sentPause = false;

namespace {
template <typename T>
void safeRelease(T *&p)
{
    if (p) {
        p->Release();
        p = nullptr;
    }
}
}

void WmiMediaControl::sendMediaKey(DWORD key)
{
    INPUT inputs[2] = {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = static_cast<WORD>(key);
    inputs[1] = inputs[0];
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(2, inputs, sizeof(INPUT));
}

bool WmiMediaControl::otherAudioPlaying()
{
    // Qt GUI thread is STA. Only uninitialize if we initialized COM here.
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    const bool needUninit = SUCCEEDED(hr);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
        return false;

    bool playing = false;
    const DWORD ownPid = GetCurrentProcessId();
    IMMDeviceEnumerator *enumerator = nullptr;
    IMMDevice *device = nullptr;
    IAudioSessionManager2 *manager = nullptr;
    IAudioSessionEnumerator *sessions = nullptr;

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                          __uuidof(IMMDeviceEnumerator),
                          reinterpret_cast<void **>(&enumerator));
    if (SUCCEEDED(hr))
        hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    if (SUCCEEDED(hr))
        hr = device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL,
                              nullptr, reinterpret_cast<void **>(&manager));
    if (SUCCEEDED(hr))
        hr = manager->GetSessionEnumerator(&sessions);

    int count = 0;
    if (SUCCEEDED(hr) && SUCCEEDED(sessions->GetCount(&count))) {
        for (int i = 0; i < count && !playing; ++i) {
            IAudioSessionControl *ctrl = nullptr;
            IAudioSessionControl2 *ctrl2 = nullptr;
            if (FAILED(sessions->GetSession(i, &ctrl)))
                continue;
            AudioSessionState state = AudioSessionStateInactive;
            if (SUCCEEDED(ctrl->GetState(&state)) && state == AudioSessionStateActive
                && SUCCEEDED(ctrl->QueryInterface(__uuidof(IAudioSessionControl2),
                                                  reinterpret_cast<void **>(&ctrl2)))) {
                DWORD pid = 0;
                ctrl2->GetProcessId(&pid);
                // Skip our own stream and the Windows system-sounds session.
                if (pid != ownPid && ctrl2->IsSystemSoundsSession() != S_OK)
                    playing = true;
            }
            safeRelease(ctrl2);
            safeRelease(ctrl);
        }
    }

    safeRelease(sessions);
    safeRelease(manager);
    safeRelease(device);
    safeRelease(enumerator);
    if (needUninit)
        CoUninitialize();
    return playing;
}

bool WmiMediaControl::pauseAll()
{
    // Play/Pause is a toggle: pressing it when nothing is playing would START
    // music. Only press it if another app is really outputting audio, and
    // never mute the device (that would silence the adhan itself).
    s_sentPause = false;
    if (otherAudioPlaying()) {
        sendMediaKey(VK_MEDIA_PLAY_PAUSE);
        s_sentPause = true;
        qDebug() << "Windows media paused";
    }
    return s_sentPause;
}

void WmiMediaControl::resumeAll()
{
    if (!s_sentPause)
        return;
    s_sentPause = false;
    sendMediaKey(VK_MEDIA_PLAY_PAUSE);
    qDebug() << "Windows media resumed";
}
