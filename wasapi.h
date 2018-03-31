#ifndef WASAPI_H
#define WASAPI_H

#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functional>

struct ComInit {
    ComInit();
    ~ComInit();
};

struct Wasapi {
    using RefillFunc = std::function<bool(float*, uint32_t, const WAVEFORMATEX*)>;

    Wasapi(RefillFunc refillFunc, int hnsBufferDuration = 30 * 10000);
    ~Wasapi();

private:
    static unsigned __stdcall tmpThreadFunc(void* arg);

    unsigned int threadFunc();

    HANDLE                  hThread { nullptr };
    IMMDeviceEnumerator*    mmDeviceEnumerator { nullptr };
    IMMDevice*              mmDevice { nullptr };
    IAudioClient*           audioClient { nullptr };
    IAudioRenderClient*     audioRenderClient { nullptr };
    WAVEFORMATEX*           mixFormat { nullptr };
    HANDLE                  hRefillEvent { nullptr };
    HANDLE                  hClose { nullptr };
    UINT32                  bufferFrameCount { 0 };
    RefillFunc              refillFunc {};
};

#endif // WASAPI_H
