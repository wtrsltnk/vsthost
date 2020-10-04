#include "wasapi.h"
#include "common.h"
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <windows.h>
#include <process.h>

Wasapi::Wasapi(Wasapi::RefillFunc refillFunc, int hnsBufferDuration)
{
    HRESULT hr = 0;

    hClose = CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
    hRefillEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
    this->refillFunc = refillFunc;

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&mmDeviceEnumerator));
    ASSERT_THROW(SUCCEEDED(hr), "CoCreateInstance(MMDeviceEnumerator) failed")

    hr = mmDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &mmDevice);
    ASSERT_THROW(SUCCEEDED(hr), "mmDeviceEnumerator->GetDefaultAudioEndpoint() failed")

    hr = mmDevice->Activate(__uuidof(IAudioClient), CLSCTX_INPROC_SERVER, nullptr, reinterpret_cast<void **>(&audioClient));
    ASSERT_THROW(SUCCEEDED(hr), "mmDevice->Activate() failed")

    audioClient->GetMixFormat(&mixFormat);

    hr = audioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST, hnsBufferDuration, 0, mixFormat, nullptr);
    ASSERT_THROW(SUCCEEDED(hr), "audioClient->Initialize() failed")

    hr = audioClient->GetService(__uuidof(IAudioRenderClient), reinterpret_cast<void **>(&audioRenderClient));
    ASSERT_THROW(SUCCEEDED(hr), "audioClient->GetService(IAudioRenderClient) failed")

    hr = audioClient->GetBufferSize(&bufferFrameCount);
    ASSERT_THROW(SUCCEEDED(hr), "audioClient->GetBufferSize() failed")

    hr = audioClient->SetEventHandle(hRefillEvent);
    ASSERT_THROW(SUCCEEDED(hr), "audioClient->SetEventHandle() failed")

    BYTE *data = nullptr;
    hr = audioRenderClient->GetBuffer(bufferFrameCount, &data);
    ASSERT_THROW(SUCCEEDED(hr), "audioRenderClient->GetBuffer() failed")

    hr = audioRenderClient->ReleaseBuffer(bufferFrameCount, AUDCLNT_BUFFERFLAGS_SILENT);
    ASSERT_THROW(SUCCEEDED(hr), "audioRenderClient->ReleaseBuffer() failed")

    unsigned threadId = 0;
    hThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, tmpThreadFunc, reinterpret_cast<void *>(this), 0, &threadId));

    hr = audioClient->Start();
    ASSERT_THROW(SUCCEEDED(hr), "audioClient->Start() failed")
}

Wasapi::~Wasapi()
{
    if (hClose)
    {
        SetEvent(hClose);
        if (hThread)
        {
            WaitForSingleObject(hThread, INFINITE);
        }
    }

    CLOSE_HANDLE(hThread)
    CLOSE_HANDLE(hClose)
    CLOSE_HANDLE(hRefillEvent)

    if (mixFormat)
    {
        CoTaskMemFree(mixFormat);
        mixFormat = nullptr;
    }

    RELEASE(audioRenderClient)
    RELEASE(audioClient)
    RELEASE(mmDevice)
    RELEASE(mmDeviceEnumerator)
}

unsigned __stdcall Wasapi::tmpThreadFunc(void *arg)
{
    return static_cast<unsigned int>((reinterpret_cast<Wasapi *>(arg))->threadFunc());
}

unsigned int Wasapi::threadFunc()
{
    const HANDLE events[2] = {hClose, hRefillEvent};
    for (bool run = true; run;)
    {
        const auto r = WaitForMultipleObjects(_countof(events), events, FALSE, INFINITE);
        if (WAIT_OBJECT_0 == r)
        { // hClose
            run = false;
        }
        else if (WAIT_OBJECT_0 + 1 == r)
        { // hRefillEvent
            UINT32 c = 0;
            audioClient->GetCurrentPadding(&c);

            const auto a = bufferFrameCount - c;
            float *data = nullptr;
            audioRenderClient->GetBuffer(a, reinterpret_cast<BYTE **>(&data));

            const auto r = refillFunc(data, a, mixFormat);
            audioRenderClient->ReleaseBuffer(a, r ? 0 : AUDCLNT_BUFFERFLAGS_SILENT);
        }
    }
    return 0;
}
