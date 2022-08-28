// Win32 : Minimal VST 2.x Synth host in C++11.
//
// This is a simplified version of the following project by hotwatermorning :
//      A sample VST Host Application for C++ Advent Calendar 2013 5th day.
//      https://github.com/hotwatermorning/VstHostDemo
//
// Usage :
//      1. Compile & Run this program.
//      2. Select your VST Synth DLL.
//      3. Press QWERTY, ZXCV, etc.
//      4. Press Ctrl-C to exit.
//
// See also :
//      Steinberg Media Technologies - The Yvan Grabit Developer Resource
//      http://ygrabit.steinberg.de/~ygrabit/public_html/index.html
//
//      MrsWatson - a command-line audio plugin host.
//      https://github.com/teragonaudio/MrsWatson
//
//  Notes :
//      "vst2.x/aeffectx.h" is part of VST3 SDK. You can download VST3 SDK from
//      Steinberg SDK Download Portal : http://www.steinberg.net/nc/en/company/developers/sdk_download_portal.html

#include <audioclient.h>
#include <mmdeviceapi.h>
#include <process.h>
#include <windows.h>

#include <algorithm>
#include <atomic>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#pragma warning(push)
#pragma warning(disable : 4996)
#include "../VST3 SDK/pluginterfaces/vst2.x/aeffectx.h"
#pragma warning(pop)

#define ASSERT_THROW(c, e)           \
    if (!(c))                        \
    {                                \
        throw std::runtime_error(e); \
    }
#define CLOSE_HANDLE(x) \
    if ((x))            \
    {                   \
        CloseHandle(x); \
        x = nullptr;    \
    }
#define RELEASE(x)      \
    if ((x))            \
    {                   \
        (x)->Release(); \
        x = nullptr;    \
    }

class VstPlugin
{
public:
    VstPlugin(
        const wchar_t *vstModulePath,
        HWND hWndParent)
    {
        init(vstModulePath, hWndParent);
    }

    ~VstPlugin()
    {
        cleanup();
    }

    size_t getSamplePos() const { return samplePos; }
    size_t getSampleRate() const { return 44100; }
    size_t getBlockSize() const { return 1024; }
    size_t getChannelCount() const { return 2; }
    static const char *getVendorString() { return "TEST_VENDOR"; }
    static const char *getProductString() { return "TEST_PRODUCT"; }
    static int getVendorVersion() { return 1; }
    static const char **getCapabilities()
    {
        static const char *hostCapabilities[] = {
            "sendVstEvents",
            "sendVstMidiEvents",
            "sizeWindow",
            "startStopProcess",
            "sendVstMidiEventFlagIsRealtime",
            nullptr};
        return hostCapabilities;
    }

    bool getFlags(int32_t m) const { return (aEffect->flags & m) == m; }
    bool flagsHasEditor() const { return getFlags(effFlagsHasEditor); }
    bool flagsIsSynth() const { return getFlags(effFlagsIsSynth); }

    intptr_t dispatcher(
        int32_t opcode,
        int32_t index = 0,
        intptr_t value = 0,
        void *ptr = nullptr,
        float opt = 0.0f) const
    {
        return aEffect->dispatcher(aEffect, opcode, index, value, ptr, opt);
    }

    void resizeEditor(
        const RECT &clientRc) const
    {
        if (editorHwnd)
        {
            auto rc = clientRc;
            const auto style = GetWindowLongPtr(editorHwnd, GWL_STYLE);
            const auto exStyle = GetWindowLongPtr(editorHwnd, GWL_EXSTYLE);
            const BOOL fMenu = GetMenu(editorHwnd) != nullptr;
            AdjustWindowRectEx(&rc, style, fMenu, exStyle);
            MoveWindow(editorHwnd, 0, 0, rc.right - rc.left, rc.bottom - rc.top, TRUE);
        }
    }

    void sendMidiNote(
        int midiChannel,
        int noteNumber,
        bool onOff,
        int velocity)
    {
        VstMidiEvent e{};
        e.type = kVstMidiType;
        e.byteSize = sizeof(e);
        e.flags = kVstMidiEventIsRealtime;
        e.midiData[0] = static_cast<char>(midiChannel + (onOff ? 0x90 : 0x80));
        e.midiData[1] = static_cast<char>(noteNumber);
        e.midiData[2] = static_cast<char>(velocity);
        if (auto l = vstMidi.lock())
        {
            vstMidi.events.push_back(e);
        }
    }

    // This function is called from refillCallback() which is running in audio thread.
    void processEvents()
    {
        vstMidiEvents.clear();
        if (auto l = vstMidi.lock())
        {
            std::swap(vstMidiEvents, vstMidi.events);
        }
        if (!vstMidiEvents.empty())
        {
            const auto n = vstMidiEvents.size();
            const auto bytes = sizeof(VstEvents) + sizeof(VstEvent *) * n;
            vstEventBuffer.resize(bytes);
            auto *ve = reinterpret_cast<VstEvents *>(vstEventBuffer.data());
            ve->numEvents = static_cast<VstInt32>(n);
            ve->reserved = 0;
            for (size_t i = 0; i < n; ++i)
            {
                ve->events[i] = reinterpret_cast<VstEvent *>(&vstMidiEvents[i]);
            }
            dispatcher(effProcessEvents, 0, 0, ve);
        }
    }

    // This function is called from refillCallback() which is running in audio thread.
    float **processAudio(
        size_t frameCount,
        size_t &outputFrameCount)
    {
        frameCount = std::min<size_t>(frameCount, outputBuffer.size() / getChannelCount());
        aEffect->processReplacing(aEffect, inputBufferHeads.data(), outputBufferHeads.data(), frameCount);
        samplePos += frameCount;
        outputFrameCount = frameCount;
        return outputBufferHeads.data();
    }

private:
    bool init(
        const wchar_t *vstModulePath,
        HWND hWndParent)
    {
        {
            wchar_t buf[MAX_PATH + 1]{};
            wchar_t *namePtr = nullptr;
            const auto r = GetFullPathName(vstModulePath, _countof(buf), buf, &namePtr);
            if (r && namePtr)
            {
                *namePtr = 0;
                char mbBuf[_countof(buf) * 4]{};
                if (auto s = WideCharToMultiByte(CP_OEMCP, 0, buf, -1, mbBuf, sizeof(mbBuf), 0, 0))
                {
                    directoryMultiByte = mbBuf;
                }
            }
        }

        hModule = LoadLibrary(vstModulePath);
        ASSERT_THROW(hModule, "Can't open VST DLL");

        typedef AEffect *(VstEntryProc)(audioMasterCallback);
        auto *vstEntryProc = reinterpret_cast<VstEntryProc *>(GetProcAddress(hModule, "VSTPluginMain"));
        if (!vstEntryProc)
        {
            vstEntryProc = reinterpret_cast<VstEntryProc *>(GetProcAddress(hModule, "main"));
        }
        ASSERT_THROW(vstEntryProc, "VST's entry point not found");

        aEffect = vstEntryProc(hostCallback_static);
        ASSERT_THROW(aEffect && aEffect->magic == kEffectMagic, "Not a VST plugin");
        ASSERT_THROW(flagsIsSynth(), "Not a VST Synth");
        aEffect->user = this;

        inputBuffer.resize(aEffect->numInputs * getBlockSize());
        for (int i = 0; i < aEffect->numInputs; ++i)
        {
            inputBufferHeads.push_back(&inputBuffer[i * getBlockSize()]);
        }

        outputBuffer.resize(aEffect->numOutputs * getBlockSize());
        for (int i = 0; i < aEffect->numOutputs; ++i)
        {
            outputBufferHeads.push_back(&outputBuffer[i * getBlockSize()]);
        }

        dispatcher(effOpen);
        dispatcher(effSetSampleRate, 0, 0, 0, static_cast<float>(getSampleRate()));
        dispatcher(effSetBlockSize, 0, getBlockSize());
        dispatcher(effSetProcessPrecision, 0, kVstProcessPrecision32);
        dispatcher(effMainsChanged, 0, 1);
        dispatcher(effStartProcess);

        if (hWndParent && flagsHasEditor())
        {
            WNDCLASSEX wcex{sizeof(wcex)};
            wcex.lpfnWndProc = DefWindowProc;
            wcex.hInstance = GetModuleHandle(0);
            wcex.lpszClassName = L"Minimal VST host - Guest VST Window Frame";
            RegisterClassEx(&wcex);

            const auto style = WS_CAPTION | WS_THICKFRAME | WS_OVERLAPPEDWINDOW;
            editorHwnd = CreateWindow(
                wcex.lpszClassName, vstModulePath, style, 0, 0, 0, 0, hWndParent, 0, 0, 0);
            dispatcher(effEditOpen, 0, 0, editorHwnd);
            RECT rc{};
            ERect *erc = nullptr;
            dispatcher(effEditGetRect, 0, 0, &erc);
            rc.left = erc->left;
            rc.top = erc->top;
            rc.right = erc->right;
            rc.bottom = erc->bottom;
            resizeEditor(rc);
            ShowWindow(editorHwnd, SW_SHOW);
        }

        return true;
    }

    void cleanup()
    {
        if (editorHwnd)
        {
            DestroyWindow(editorHwnd);

            dispatcher(effEditClose);
            editorHwnd = nullptr;
        }
        dispatcher(effStopProcess);
        dispatcher(effMainsChanged, 0, 0);
        dispatcher(effClose);
        if (hModule)
        {
            FreeLibrary(hModule);
            hModule = nullptr;
        }
    }

    static VstIntPtr hostCallback_static(
        AEffect *effect,
        VstInt32 opcode,
        VstInt32 index,
        VstIntPtr value,
        void *ptr,
        float opt)
    {
        if (effect && effect->user)
        {
            auto *that = static_cast<VstPlugin *>(effect->user);
            return that->hostCallback(opcode, index, value, ptr, opt);
        }

        switch (opcode)
        {
            case audioMasterVersion:
                return kVstVersion;
            default:
                return 0;
        }
    }

    VstIntPtr hostCallback(
        VstInt32 opcode,
        VstInt32 index,
        VstIntPtr value,
        void *ptr,
        float)
    {
        switch (opcode)
        {
            default:
                break;
            case audioMasterVersion:
                return kVstVersion;
            case audioMasterCurrentId:
                return aEffect->uniqueID;
            case audioMasterGetSampleRate:
                return getSampleRate();
            case audioMasterGetBlockSize:
                return getBlockSize();
            case audioMasterGetCurrentProcessLevel:
                return kVstProcessLevelUnknown;
            case audioMasterGetAutomationState:
                return kVstAutomationOff;
            case audioMasterGetLanguage:
                return kVstLangEnglish;
            case audioMasterGetVendorVersion:
                return getVendorVersion();

            case audioMasterGetVendorString:
                strcpy_s(static_cast<char *>(ptr), kVstMaxVendorStrLen, getVendorString());
                return 1;

            case audioMasterGetProductString:
                strcpy_s(static_cast<char *>(ptr), kVstMaxProductStrLen, getProductString());
                return 1;

            case audioMasterGetTime:
                timeinfo.flags = 0;
                timeinfo.samplePos = getSamplePos();
                timeinfo.sampleRate = getSampleRate();
                return reinterpret_cast<VstIntPtr>(&timeinfo);

            case audioMasterGetDirectory:
                return reinterpret_cast<VstIntPtr>(directoryMultiByte.c_str());

            case audioMasterIdle:
                if (editorHwnd)
                {
                    dispatcher(effEditIdle);
                }
                break;

            case audioMasterSizeWindow:
                if (editorHwnd)
                {
                    RECT rc{};
                    GetWindowRect(editorHwnd, &rc);
                    rc.right = rc.left + static_cast<int>(index);
                    rc.bottom = rc.top + static_cast<int>(value);
                    resizeEditor(rc);
                }
                break;

            case audioMasterCanDo:
                for (const char **pp = getCapabilities(); *pp; ++pp)
                {
                    if (strcmp(*pp, static_cast<const char *>(ptr)) == 0)
                    {
                        return 1;
                    }
                }
                return 0;
        }
        return 0;
    }

protected:
    HWND editorHwnd{nullptr};
    HMODULE hModule{nullptr};
    AEffect *aEffect{nullptr};
    std::atomic<size_t> samplePos{0};
    VstTimeInfo timeinfo{};
    std::string directoryMultiByte{};

    std::vector<float> outputBuffer;
    std::vector<float *> outputBufferHeads;
    std::vector<float> inputBuffer;
    std::vector<float *> inputBufferHeads;

    std::vector<VstMidiEvent> vstMidiEvents;
    std::vector<char> vstEventBuffer;

    struct
    {
        std::vector<VstMidiEvent> events;
        std::unique_lock<std::mutex> lock() const
        {
            return std::unique_lock<std::mutex>(mutex);
        }

    private:
        std::mutex mutable mutex;
    } vstMidi;
};

struct ComInit
{
    ComInit() { CoInitializeEx(nullptr, COINIT_MULTITHREADED); }
    ~ComInit() { CoUninitialize(); }
};

struct Wasapi
{
    using RefillFunc = std::function<bool(float *, uint32_t, const WAVEFORMATEX *)>;

    Wasapi(RefillFunc refillFunc, int hnsBufferDuration = 30 * 10000)
    {
        HRESULT hr = S_OK;

        hClose = CreateEventEx(0, 0, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
        hRefillEvent = CreateEventEx(0, 0, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
        this->refillFunc = refillFunc;

        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), 0, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&mmDeviceEnumerator));
        ASSERT_THROW(SUCCEEDED(hr), "CoCreateInstance(MMDeviceEnumerator) failed");

        hr = mmDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &mmDevice);
        ASSERT_THROW(SUCCEEDED(hr), "mmDeviceEnumerator->GetDefaultAudioEndpoint() failed");

        hr = mmDevice->Activate(__uuidof(IAudioClient), CLSCTX_INPROC_SERVER, 0, reinterpret_cast<void **>(&audioClient));
        ASSERT_THROW(SUCCEEDED(hr), "mmDevice->Activate() failed");

        audioClient->GetMixFormat(&mixFormat);

        hr = audioClient->Initialize(
            AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST, hnsBufferDuration, 0, mixFormat, nullptr);
        ASSERT_THROW(SUCCEEDED(hr), "audioClient->Initialize() failed");

        hr = audioClient->GetService(__uuidof(IAudioRenderClient), reinterpret_cast<void **>(&audioRenderClient));
        ASSERT_THROW(SUCCEEDED(hr), "audioClient->GetService(IAudioRenderClient) failed");

        hr = audioClient->GetBufferSize(&bufferFrameCount);
        ASSERT_THROW(SUCCEEDED(hr), "audioClient->GetBufferSize() failed");

        hr = audioClient->SetEventHandle(hRefillEvent);
        ASSERT_THROW(SUCCEEDED(hr), "audioClient->SetEventHandle() failed");

        BYTE *data = nullptr;
        hr = audioRenderClient->GetBuffer(bufferFrameCount, &data);
        ASSERT_THROW(SUCCEEDED(hr), "audioRenderClient->GetBuffer() failed");

        hr = audioRenderClient->ReleaseBuffer(bufferFrameCount, AUDCLNT_BUFFERFLAGS_SILENT);
        ASSERT_THROW(SUCCEEDED(hr), "audioRenderClient->ReleaseBuffer() failed");

        unsigned threadId = 0;
        hThread = reinterpret_cast<HANDLE>(_beginthreadex(0, 0, tmpThreadFunc, reinterpret_cast<void *>(this), 0, &threadId));

        hr = audioClient->Start();
        ASSERT_THROW(SUCCEEDED(hr), "audioClient->Start() failed");
    }

    ~Wasapi()
    {
        if (hClose)
        {
            SetEvent(hClose);
            if (hThread)
            {
                WaitForSingleObject(hThread, INFINITE);
            }
        }

        CLOSE_HANDLE(hThread);
        CLOSE_HANDLE(hClose);
        CLOSE_HANDLE(hRefillEvent);

        if (mixFormat)
        {
            CoTaskMemFree(mixFormat);
            mixFormat = nullptr;
        }

        RELEASE(audioRenderClient);
        RELEASE(audioClient);
        RELEASE(mmDevice);
        RELEASE(mmDeviceEnumerator);
    }

private:
    static unsigned __stdcall tmpThreadFunc(void *arg)
    {
        return (uint32_t)(reinterpret_cast<Wasapi *>(arg))->threadFunc();
    }

    uint32_t threadFunc()
    {
        ComInit comInit{};
        bool flag = false;
        const HANDLE events[2] = {hClose, hRefillEvent};
        for (bool run = true; run;)
        {
            const auto r = WaitForMultipleObjects(_countof(events), events, FALSE, INFINITE);
            if (WAIT_OBJECT_0 == r)
            { // hClose
                if (flag != run)
                    continue;
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

    HANDLE hThread{nullptr};
    IMMDeviceEnumerator *mmDeviceEnumerator{nullptr};
    IMMDevice *mmDevice{nullptr};
    IAudioClient *audioClient{nullptr};
    IAudioRenderClient *audioRenderClient{nullptr};
    WAVEFORMATEX *mixFormat{nullptr};
    HANDLE hRefillEvent{nullptr};
    HANDLE hClose{nullptr};
    UINT32 bufferFrameCount{0};
    RefillFunc refillFunc{};
};

// This function is called from Wasapi::threadFunc() which is running in audio thread.
bool refillCallback(VstPlugin &vstPlugin, float *const data, uint32_t availableFrameCount, const WAVEFORMATEX *const mixFormat)
{
    vstPlugin.processEvents();

    const auto nDstChannels = mixFormat->nChannels;
    const auto nSrcChannels = vstPlugin.getChannelCount();
    const auto vstSamplesPerBlock = vstPlugin.getBlockSize();

    int ofs = 0;
    while (availableFrameCount > 0)
    {
        size_t outputFrameCount = 0;
        float **vstOutput = vstPlugin.processAudio(availableFrameCount, outputFrameCount);

        // VST vstOutput[][] format :
        //  vstOutput[a][b]
        //      channel = a % vstPlugin.getChannelCount()
        //      frame   = b + floor(a/2) * vstPlugin.getBlockSize()

        // wasapi data[] format :
        //  data[x]
        //      channel = x % mixFormat->nChannels
        //      frame   = floor(x / mixFormat->nChannels);

        const auto nFrame = outputFrameCount;
        for (size_t iFrame = 0; iFrame < nFrame; ++iFrame)
        {
            for (size_t iChannel = 0; iChannel < nDstChannels; ++iChannel)
            {
                const int sChannel = iChannel % nSrcChannels;
                const int vstOutputPage = (iFrame / vstSamplesPerBlock) * sChannel + sChannel;
                const int vstOutputIndex = (iFrame % vstSamplesPerBlock);
                const int wasapiWriteIndex = iFrame * nDstChannels + iChannel;
                *(data + ofs + wasapiWriteIndex) = vstOutput[vstOutputPage][vstOutputIndex];
            }
        }

        availableFrameCount -= nFrame;
        ofs += nFrame * nDstChannels;
    }
    return true;
}

HDC dc;
HGLRC rc;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CLOSE:
        {
            DestroyWindow(hwnd);
            break;
        }
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            break;
        }
        default:
        {
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    }
    return 0;
}

const wchar_t g_szClassName[] = L"myWindowClass";

void mainLoop(const std::wstring &dllFilename)
{
    WNDCLASSEX wc;
    HWND hwnd;

    //Step 1: Registering the Window Class
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = g_szClassName;
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc))
    {
        MessageBox(NULL, L"Window Registration Failed!", L"Error!",
                   MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    // Step 2: Creating the Window
    hwnd = CreateWindow(
        g_szClassName,
        L"The title of my window",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 240, 120,
        NULL, NULL, GetModuleHandle(NULL), NULL);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    PIXELFORMATDESCRIPTOR format_desc;
    format_desc.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    format_desc.nVersion = 1;
    format_desc.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;

    dc = GetDC(hwnd);
    int format = ChoosePixelFormat(dc, &format_desc);
    SetPixelFormat(dc, format, &format_desc);
    rc = wglCreateContext(dc);
    wglMakeCurrent(dc, rc);

    VstPlugin vstPlugin{dllFilename.c_str(), hwnd};
    Wasapi wasapi{[&vstPlugin](float *const data, uint32_t availableFrameCount, const WAVEFORMATEX *const mixFormat) {
        return refillCallback(vstPlugin, data, availableFrameCount, mixFormat);
    }};

    struct Key
    {
        Key(int midiNote)
            : midiNote{midiNote}
        {
        }
        int midiNote{};
        bool status{false};
    };

    std::map<int, Key> keyMap{
        {'2', {61}},
        {'3', {63}},
        {'5', {66}},
        {'6', {68}},
        {'7', {70}},
        {'Q', {60}},
        {'W', {62}},
        {'E', {64}},
        {'R', {65}},
        {'T', {67}},
        {'Y', {69}},
        {'U', {71}},
        {'I', {72}},

        {'S', {49}},
        {'D', {51}},
        {'G', {54}},
        {'H', {56}},
        {'J', {58}},
        {'Z', {48}},
        {'X', {50}},
        {'C', {52}},
        {'V', {53}},
        {'B', {55}},
        {'N', {57}},
        {'M', {59}},
        {VK_OEM_COMMA, {60}},
    };

    for (bool run = true; run; WaitMessage())
    {
        MSG msg{};
        while (BOOL b = PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            if (b == -1)
            {
                run = false;
                break;
            }

            if (msg.message == WM_QUIT)
            {
                run = false;
                break;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (msg.message == WM_QUIT)
        {
            run = false;
            break;
        }

        for (auto &e : keyMap)
        {
            auto &key = e.second;
            const auto on = (GetKeyState(e.first) & 0x8000) != 0;
            if (key.status != on)
            {
                key.status = on;
                vstPlugin.sendMidiNote(0, key.midiNote, on, 100);
            }
        }
    }
}

int main()
{
    ComInit comInit{};

    const auto dllFilename = []() -> std::wstring {
        wchar_t fn[MAX_PATH + 1]{};
        OPENFILENAME ofn;
        memset(&ofn, 0, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFilter = L"VSTi DLL(*.dll)\0*.dll\0All Files(*.*)\0*.*\0\0";
        ofn.lpstrFile = fn;
        ofn.nMaxFile = _countof(fn);
        ofn.lpstrTitle = L"Select VST DLL";
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_ENABLESIZING;
        GetOpenFileName(&ofn);
        return fn;
    }();

    try
    {
        mainLoop(dllFilename);
    }
    catch (std::exception &e)
    {
        std::cout << "Exception : " << e.what() << std::endl;
    }
}
