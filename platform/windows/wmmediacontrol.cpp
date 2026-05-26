#include "wmmediacontrol.h"
#include <QDebug>
#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>

bool WmiMediaControl::s_muted = false;
int WmiMediaControl::s_savedVolume = 100;

void WmiMediaControl::sendMediaKey(DWORD key)
{
    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = key;
    SendInput(1, &input, sizeof(INPUT));
    input.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

void WmiMediaControl::muteSystem(bool mute)
{
    HRESULT hr;
    IMMDeviceEnumerator *enumerator = nullptr;
    IMMDevice *device = nullptr;
    IAudioEndpointVolume *endpoint = nullptr;

    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) return;

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                          CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                          (void**)&enumerator);
    if (SUCCEEDED(hr)) {
        hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
        if (SUCCEEDED(hr)) {
            hr = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL,
                                  nullptr, (void**)&endpoint);
            if (SUCCEEDED(hr)) {
                if (mute) {
                    endpoint->GetMasterVolumeLevelScalar((float*)&s_savedVolume);
                    endpoint->SetMute(TRUE, nullptr);
                } else {
                    endpoint->SetMute(FALSE, nullptr);
                    endpoint->SetMasterVolumeLevelScalar(s_savedVolume / 100.0f, nullptr);
                }
                endpoint->Release();
            }
            device->Release();
        }
        enumerator->Release();
    }
    CoUninitialize();
}

void WmiMediaControl::pauseAll()
{
    sendMediaKey(VK_MEDIA_PLAY_PAUSE);
    muteSystem(true);
    s_muted = true;
    qDebug() << "Windows media paused + muted";
}

void WmiMediaControl::resumeAll()
{
    if (s_muted) {
        muteSystem(false);
        s_muted = false;
    }
    sendMediaKey(VK_MEDIA_PLAY_PAUSE);
    qDebug() << "Windows media resumed + unmuted";
}
