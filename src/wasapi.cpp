#include "wasapi.h"

#include <stdexcept>

#define ASSERT_THROW(c,e)   if(!(c)) { throw std::runtime_error(e); }
#define CLOSE_HANDLE(x)     if((x)) { CloseHandle(x); x = nullptr; }
#define RELEASE(x)          if((x)) { (x)->Release(); x = nullptr; }

#include <audioclient.h>
#include <mmdeviceapi.h>
#include <process.h>
#include <windows.h>

#include <AudioClient.h>
#include <AudioPolicy.h>
#include <MMDeviceAPI.h>
#include <avrt.h>
#include <functiondiscoverykeys.h>

std::wstring GetDeviceName(
    IMMDevice *device)
{
    LPWSTR deviceId;
    HRESULT hr;

    hr = device->GetId(&deviceId);
    ASSERT_THROW(SUCCEEDED(hr), "device->GetId failed");

    IPropertyStore *propertyStore;
    hr = device->OpenPropertyStore(STGM_READ, &propertyStore);
    ASSERT_THROW(SUCCEEDED(hr), "device->OpenPropertyStore failed");

    PROPVARIANT friendlyName;
    PropVariantInit(&friendlyName);
    hr = propertyStore->GetValue(PKEY_Device_FriendlyName, &friendlyName);
    RELEASE(propertyStore);
    ASSERT_THROW(SUCCEEDED(hr), "propertyStore->GetValue failed");

    auto Result = std::wstring(friendlyName.pwszVal); // + String(" (") + String( UnicodeString(deviceId) ) + String(")")

    PropVariantClear(&friendlyName);
    CoTaskMemFree(deviceId);

    return Result;
}

std::wstring GetDeviceNameByIndex(
    IMMDeviceCollection *DeviceCollection,
    UINT DeviceIndex)
{
    IMMDevice *device;
    HRESULT hr;

    hr = DeviceCollection->Item(DeviceIndex, &device);
    ASSERT_THROW(SUCCEEDED(hr), "DeviceCollection->Item failed");

    auto result = GetDeviceName(device);

    RELEASE(device);

    return result;
}

Wasapi::Wasapi(
    Wasapi::RefillFunc refillFunc,
    int hnsBufferDuration)
    : _hnsBufferDuration(hnsBufferDuration)
{
    this->refillFunc = refillFunc;

    HRESULT hr = 0;

    _hClose = CreateEventEx(
        nullptr,
        nullptr,
        0,
        EVENT_MODIFY_STATE | SYNCHRONIZE);
    _hRefillEvent = CreateEventEx(
        nullptr,
        nullptr,
        0,
        EVENT_MODIFY_STATE | SYNCHRONIZE);

    IMMDeviceEnumerator *mmDeviceEnumerator;

    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&mmDeviceEnumerator));
    ASSERT_THROW(SUCCEEDED(hr), "CoCreateInstance(MMDeviceEnumerator) failed")

    IMMDeviceCollection *deviceCollection = NULL;

    hr = mmDeviceEnumerator->EnumAudioEndpoints(
        eRender,
        DEVICE_STATE_ACTIVE,
        &deviceCollection);
    ASSERT_THROW(SUCCEEDED(hr), "mmDeviceEnumerator->EnumAudioEndpoints failed");

    hr = deviceCollection->GetCount(&_deviceCount);
    ASSERT_THROW(SUCCEEDED(hr), "deviceCollection->GetCount failed");
    for (UINT DeviceIndex = 0; DeviceIndex < _deviceCount; DeviceIndex++)
    {
        _devices.push_back(GetDeviceNameByIndex(deviceCollection, DeviceIndex));
    }

    hr = mmDeviceEnumerator->GetDefaultAudioEndpoint(
        eRender,
        eMultimedia,
        &_mmDevice);
    ASSERT_THROW(SUCCEEDED(hr), "mmDeviceEnumerator->GetDefaultAudioEndpoint() failed")

    _currentDevice = GetDeviceName(_mmDevice);

    RELEASE(mmDeviceEnumerator)

    ActivateAndStartDevice(hnsBufferDuration);
}

Wasapi::~Wasapi()
{
    StopAndCleanupDevice();
}

void Wasapi::SelectDevice(
    uint32_t index)
{
    if (index < 0)
    {
        return;
    }

    if (_deviceCount <= index)
    {
        return;
    }

    StopAndCleanupDevice();

    HRESULT hr = 0;

    _hClose = CreateEventEx(
        nullptr,
        nullptr,
        0,
        EVENT_MODIFY_STATE | SYNCHRONIZE);
    _hRefillEvent = CreateEventEx(
        nullptr,
        nullptr,
        0,
        EVENT_MODIFY_STATE | SYNCHRONIZE);

    IMMDeviceCollection *deviceCollection = NULL;

    IMMDeviceEnumerator *mmDeviceEnumerator;

    hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&mmDeviceEnumerator));
    ASSERT_THROW(SUCCEEDED(hr), "CoCreateInstance(MMDeviceEnumerator) failed")

    hr = mmDeviceEnumerator->EnumAudioEndpoints(
        eCapture,
        DEVICE_STATE_ACTIVE,
        &deviceCollection);
    ASSERT_THROW(SUCCEEDED(hr), "mmDeviceEnumerator->EnumAudioEndpoints failed");

    hr = deviceCollection->GetCount(&_deviceCount);
    ASSERT_THROW(SUCCEEDED(hr), "deviceCollection->GetCount failed");

    hr = deviceCollection->Item(index, &_mmDevice);
    ASSERT_THROW(SUCCEEDED(hr), "deviceCollection->Item failed");

    ActivateAndStartDevice(_hnsBufferDuration);

    RELEASE(mmDeviceEnumerator)
}

void Wasapi::ActivateAndStartDevice(
    int hnsBufferDuration)
{
    HRESULT hr = 0;

    hr = _mmDevice->Activate(
        __uuidof(IAudioClient),
        CLSCTX_INPROC_SERVER,
        nullptr,
        reinterpret_cast<void **>(&_audioClient));
    ASSERT_THROW(SUCCEEDED(hr), "mmDevice->Activate() failed")

    _audioClient->GetMixFormat(&_mixFormat);

    hr = _audioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST,
        hnsBufferDuration,
        0,
        _mixFormat,
        nullptr);
    ASSERT_THROW(SUCCEEDED(hr), "audioClient->Initialize() failed")

    hr = _audioClient->GetService(
        __uuidof(IAudioRenderClient),
        reinterpret_cast<void **>(&_audioRenderClient));
    ASSERT_THROW(SUCCEEDED(hr), "audioClient->GetService(IAudioRenderClient) failed")

    hr = _audioClient->GetBufferSize(
        &_bufferFrameCount);
    ASSERT_THROW(SUCCEEDED(hr), "audioClient->GetBufferSize() failed")

    hr = _audioClient->SetEventHandle(
        _hRefillEvent);
    ASSERT_THROW(SUCCEEDED(hr), "audioClient->SetEventHandle() failed")

    BYTE *data = nullptr;
    hr = _audioRenderClient->GetBuffer(
        _bufferFrameCount,
        &data);
    ASSERT_THROW(SUCCEEDED(hr), "audioRenderClient->GetBuffer() failed")

    hr = _audioRenderClient->ReleaseBuffer(
        _bufferFrameCount,
        AUDCLNT_BUFFERFLAGS_SILENT);
    ASSERT_THROW(SUCCEEDED(hr), "audioRenderClient->ReleaseBuffer() failed")

    unsigned threadId = 0;
    _hThread = reinterpret_cast<HANDLE>(_beginthreadex(nullptr, 0, tmpThreadFunc, reinterpret_cast<void *>(this), 0, &threadId));

    hr = _audioClient->Start();
    ASSERT_THROW(SUCCEEDED(hr), "audioClient->Start() failed")
}

void Wasapi::StopAndCleanupDevice()
{
    if (_hClose)
    {
        SetEvent(_hClose);
        if (_hThread)
        {
            WaitForSingleObject(_hThread, INFINITE);
        }
    }

    CLOSE_HANDLE(_hThread)
    CLOSE_HANDLE(_hClose)
    CLOSE_HANDLE(_hRefillEvent)

    if (_mixFormat)
    {
        CoTaskMemFree(_mixFormat);
        _mixFormat = nullptr;
    }

    RELEASE(_audioRenderClient)
    RELEASE(_audioClient)
    RELEASE(_mmDevice)
}
const std::wstring &Wasapi::CurrentDevice() const
{
    return _currentDevice;
}

const std::vector<std::wstring> &Wasapi::Devices() const
{
    return _devices;
}

unsigned __stdcall Wasapi::tmpThreadFunc(
    void *arg)
{
    return static_cast<uint32_t>((reinterpret_cast<Wasapi *>(arg))->threadFunc());
}

uint32_t Wasapi::threadFunc()
{
    const HANDLE events[2] = {_hClose, _hRefillEvent};
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
            _audioClient->GetCurrentPadding(&c);

            const auto a = _bufferFrameCount - c;
            float *data = nullptr;
            _audioRenderClient->GetBuffer(a, reinterpret_cast<BYTE **>(&data));

            const auto r = refillFunc(data, a, _mixFormat);
            _audioRenderClient->ReleaseBuffer(a, r ? 0 : AUDCLNT_BUFFERFLAGS_SILENT);
        }
    }
    return 0;
}
