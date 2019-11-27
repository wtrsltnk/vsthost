#ifndef WASAPI_H
#define WASAPI_H

#include <audioclient.h>
#include <functional>
#include <mmdeviceapi.h>
#include <windows.h>

struct Wasapi
{
    using RefillFunc = std::function<bool(float *, uint32_t, const WAVEFORMATEX *)>;

    Wasapi(RefillFunc refillFunc, int hnsBufferDuration = 30 * 10000);
    ~Wasapi();

private:
    static unsigned __stdcall tmpThreadFunc(void *arg);

    unsigned int threadFunc();

    HANDLE hThread;
    IMMDeviceEnumerator *mmDeviceEnumerator;
    IMMDevice *mmDevice;
    IAudioClient *audioClient;
    IAudioRenderClient *audioRenderClient;
    WAVEFORMATEX *mixFormat;
    HANDLE hRefillEvent;
    HANDLE hClose;
    UINT32 bufferFrameCount;
    RefillFunc refillFunc;
};

#endif // WASAPI_H
