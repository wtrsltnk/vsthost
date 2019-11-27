#include "vstplugin.h"
#include "common.h"
#include <iostream>

VstPlugin::VstPlugin(const wchar_t *vstModulePath)
    : editorHwnd(nullptr), hModule(nullptr), aEffect(nullptr)
{
    init(vstModulePath);
}

VstPlugin::~VstPlugin()
{
    cleanup();
}

const char **VstPlugin::getCapabilities()
{
    static const char *hostCapabilities[] = {
        "sendVstEvents",
        "sendVstMidiEvents",
        "sizeWindow",
        "startStopProcess",
        "sendVstMidiEventFlagIsRealtime",
        nullptr,
    };

    return hostCapabilities;
}

intptr_t VstPlugin::dispatcher(int32_t opcode, int32_t index, intptr_t value, void *ptr, float opt) const
{
    return aEffect->dispatcher(aEffect, opcode, index, value, ptr, opt);
}

void VstPlugin::resizeEditor(const RECT &clientRc) const
{
    if (editorHwnd == nullptr)
    {
        return;
    }

    auto rc = clientRc;
    DWORD style = static_cast<DWORD>(GetWindowLongPtr(editorHwnd, GWL_STYLE));
    DWORD exStyle = static_cast<DWORD>(GetWindowLongPtr(editorHwnd, GWL_EXSTYLE));

    AdjustWindowRectEx(&rc, style, GetMenu(editorHwnd) != nullptr, exStyle);
    MoveWindow(editorHwnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
}

LRESULT CALLBACK VstWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto vstPlugin = reinterpret_cast<VstPlugin *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (message)
    {
        case WM_CREATE:
        {
            auto createStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
            vstPlugin = reinterpret_cast<VstPlugin *>(createStruct->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
            break;
        }
        case WM_CLOSE:
        {
            vstPlugin->closingEditorWindow();
            break;
        }
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

void VstPlugin::openEditor(HWND hWndParent)
{
    if (!flagsHasEditor())
    {
        return;
    }

    if (editorHwnd == nullptr)
    {
        WNDCLASSEX wcex;
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
        wcex.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW);
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.style = 0;
        wcex.lpfnWndProc = VstWindowProc;
        wcex.hInstance = GetModuleHandle(nullptr);
        wcex.lpszClassName = L"Minimal VST host - Guest VST Window Frame";
        wcex.lpszMenuName = nullptr;
        RegisterClassEx(&wcex);

        auto xscreen = GetSystemMetrics(SM_CXSCREEN);
        auto yscreen = GetSystemMetrics(SM_CYSCREEN);

        editorHwnd = CreateWindow(
            wcex.lpszClassName,
            L"VST Plugin",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
            CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT,
            hWndParent, nullptr, nullptr,
            reinterpret_cast<LPVOID>(this));

        dispatcher(effEditOpen, 0, 0, editorHwnd);

        ERect *erc = nullptr;
        dispatcher(effEditGetRect, 0, 0, &erc);

        auto xoffset = (xscreen - (erc->right - erc->left)) / 2;
        auto yoffset = (yscreen - (erc->bottom - erc->top)) / 2;

        resizeEditor(RECT{
            xoffset + erc->left,
            yoffset + erc->top,
            xoffset + erc->right,
            yoffset + erc->bottom,
        });
    }

    ShowWindow(editorHwnd, SW_SHOW);
}
bool VstPlugin::isEditorOpen()
{
    return editorHwnd != nullptr;
}

void VstPlugin::closeEditor()
{
    HWND tmp = editorHwnd;
    closingEditorWindow();
    DestroyWindow(tmp);
}

void VstPlugin::closingEditorWindow()
{
    dispatcher(effEditClose, 0, 0, editorHwnd);
    editorHwnd = nullptr;
}

void VstPlugin::sendMidiNote(int midiChannel, int noteNumber, bool onOff, int velocity)
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
void VstPlugin::processEvents()
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
        ve->numEvents = static_cast<int>(n);
        ve->reserved = 0;
        for (size_t i = 0; i < n; ++i)
        {
            ve->events[i] = reinterpret_cast<VstEvent *>(&vstMidiEvents[i]);
        }
        dispatcher(effProcessEvents, 0, 0, ve);
    }
}

// This function is called from refillCallback() which is running in audio thread.
float **VstPlugin::processAudio(size_t frameCount, size_t &outputFrameCount)
{
    frameCount = std::min<size_t>(frameCount, outputBuffer.size() / getChannelCount());

    aEffect->processReplacing(aEffect, inputBufferHeads.data(), outputBufferHeads.data(), static_cast<int>(frameCount));
    samplePos += frameCount;
    outputFrameCount = frameCount;

    return outputBufferHeads.data();
}

bool VstPlugin::init(const wchar_t *vstModulePath)
{
    {
        wchar_t buf[MAX_PATH + 1];
        wchar_t *namePtr = nullptr;
        const auto r = GetFullPathName(vstModulePath, _countof(buf), buf, &namePtr);
        if (r && namePtr)
        {
            *namePtr = 0;
            char mbBuf[_countof(buf) * 4];
            if (auto s = WideCharToMultiByte(CP_OEMCP, 0, buf, -1, mbBuf, sizeof(mbBuf), nullptr, nullptr))
            {
                directoryMultiByte = mbBuf;
            }
        }
    }

    hModule = LoadLibrary(vstModulePath);
    ASSERT_THROW(hModule, "Can't open VST DLL")

    typedef AEffect *(VstEntryProc)(audioMasterCallback);
    auto *vstEntryProc = reinterpret_cast<VstEntryProc *>(GetProcAddress(hModule, "VSTPluginMain"));
    if (!vstEntryProc)
    {
        vstEntryProc = reinterpret_cast<VstEntryProc *>(GetProcAddress(hModule, "main"));
    }
    ASSERT_THROW(vstEntryProc, "VST's entry point not found")

    aEffect = vstEntryProc(hostCallback_static);
    ASSERT_THROW(aEffect && aEffect->magic == kEffectMagic, "Not a VST plugin")
    ASSERT_THROW(flagsIsSynth(), "Not a VST Synth")
    aEffect->user = this;

    inputBuffer.resize(static_cast<size_t>(aEffect->numInputs) * getBlockSize());
    for (int i = 0; i < aEffect->numInputs; ++i)
    {
        inputBufferHeads.push_back(&inputBuffer[static_cast<size_t>(i) * getBlockSize()]);
    }

    outputBuffer.resize(static_cast<size_t>(aEffect->numOutputs) * getBlockSize());
    for (int i = 0; i < aEffect->numOutputs; ++i)
    {
        outputBufferHeads.push_back(&outputBuffer[static_cast<size_t>(i) * getBlockSize()]);
    }

    dispatcher(effOpen);
    dispatcher(effSetSampleRate, 0, 0, nullptr, static_cast<float>(getSampleRate()));
    dispatcher(effSetBlockSize, 0, static_cast<intptr_t>(getBlockSize()));
    dispatcher(effSetProcessPrecision, 0, kVstProcessPrecision32);
    dispatcher(effMainsChanged, 0, 1);
    dispatcher(effStartProcess);

    char ceffname[kVstMaxEffectNameLen] = "";
    dispatcher(effGetEffectName, 0, 0, ceffname, 0.0f);
    vstEffectName = ceffname;

    char cvendor[kVstMaxVendorStrLen] = "";
    dispatcher(effGetVendorString, 0, 0, cvendor, 0.0f);
    vstVendorName = cvendor;

    return true;
}

void VstPlugin::cleanup()
{
    if (editorHwnd)
    {
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

VstIntPtr VstPlugin::hostCallback_static(AEffect *effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt)
{
    if (effect && effect->user)
    {
        auto *that = static_cast<VstPlugin *>(effect->user);
        return that->hostCallback(opcode, index, value, ptr, opt);
    }

    switch (opcode)
    {
        case audioMasterVersion:
        {
            return kVstVersion;
        }
        default:
        {
            return 0;
        }
    }
}

VstIntPtr VstPlugin::hostCallback(VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float)
{
    switch (opcode)
    {
        case audioMasterVersion:
        {
            return kVstVersion;
        }
        case audioMasterCurrentId:
        {
            return aEffect->uniqueID;
        }
        case audioMasterGetSampleRate:
        {
            return static_cast<int>(getSampleRate());
        }
        case audioMasterGetBlockSize:
        {
            return static_cast<int>(getBlockSize());
        }
        case audioMasterGetCurrentProcessLevel:
        {
            return kVstProcessLevelUnknown;
        }
        case audioMasterGetAutomationState:
        {
            return kVstAutomationOff;
        }
        case audioMasterGetLanguage:
        {
            return kVstLangEnglish;
        }
        case audioMasterGetVendorVersion:
        {
            return getVendorVersion();
        }
        case audioMasterGetVendorString:
        {
            strcpy_s(static_cast<char *>(ptr), kVstMaxVendorStrLen, getVendorString());
            return 1;
        }
        case audioMasterGetProductString:
        {
            strcpy_s(static_cast<char *>(ptr), kVstMaxProductStrLen, getProductString());
            return 1;
        }
        case audioMasterGetTime:
        {
            timeinfo.flags = 0;
            timeinfo.samplePos = getSamplePos();
            timeinfo.sampleRate = getSampleRate();
            return reinterpret_cast<VstIntPtr>(&timeinfo);
        }
        case audioMasterGetDirectory:
        {
            return reinterpret_cast<VstIntPtr>(directoryMultiByte.c_str());
        }
        case audioMasterIdle:
        {
            if (editorHwnd)
            {
                dispatcher(effEditIdle);
            }
            break;
        }
        case audioMasterSizeWindow:
        {
            if (editorHwnd != nullptr)
            {
                RECT rc;
                GetWindowRect(editorHwnd, &rc);

                rc.right = rc.left + static_cast<int>(index);
                rc.bottom = rc.top + static_cast<int>(value);

                resizeEditor(rc);
            }
            break;
        }
        case audioMasterCanDo:
        {
            for (const char **pp = getCapabilities(); *pp; ++pp)
            {
                if (strcmp(*pp, static_cast<const char *>(ptr)) == 0)
                {
                    return 1;
                }
            }
            return 0;
        }
        default:
        {
            break;
        }
    }
    return 0;
}
