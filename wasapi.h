#ifndef WASAPI_H
#define WASAPI_H

#include <audioclient.h>
#include <functional>
#include <mmdeviceapi.h>
#include <windows.h>

struct Wasapi
{
    using RefillFunc = std::function<bool(float *, uint32_t, const WAVEFORMATEX *)>;

    Wasapi(
        RefillFunc refillFunc,
        int hnsBufferDuration = 30 * 10000);
    ~Wasapi();

    const std::wstring &CurrentDevice() const;

    const std::vector<std::wstring> &Devices() const;

    void SelectDevice(
        uint32_t index);
private:
    static unsigned __stdcall tmpThreadFunc(
        void *arg);

    void ActivateAndStartDevice(
        int hnsBufferDuration);

    void StopAndCleanupDevice();

    unsigned int threadFunc();

    HANDLE _hThread;
    IMMDevice *_mmDevice;
    IAudioClient *_audioClient;
    IAudioRenderClient *_audioRenderClient;
    WAVEFORMATEX *_mixFormat;
    HANDLE _hRefillEvent;
    HANDLE _hClose;
    UINT32 _bufferFrameCount;
    UINT _deviceCount;
    RefillFunc refillFunc;
    int _hnsBufferDuration;
    std::wstring _currentDevice;
    std::vector<std::wstring> _devices;
};

#endif // WASAPI_H
