#include "vstplugin.h"
#include "common.h"

VstPlugin::VstPlugin(const wchar_t* vstModulePath, HWND hWndParent)
{
    init(vstModulePath, hWndParent);
}

VstPlugin::~VstPlugin()
{
    cleanup();
}

intptr_t VstPlugin::dispatcher(int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt) const
{
    return aEffect->dispatcher(aEffect, opcode, index, value, ptr, opt);
}

void VstPlugin::resizeEditor(const RECT& clientRc) const
{
    if (editorHwnd) {
        auto rc = clientRc;
        const auto style = GetWindowLongPtr(editorHwnd, GWL_STYLE);
        const auto exStyle = GetWindowLongPtr(editorHwnd, GWL_EXSTYLE);
        const BOOL fMenu = GetMenu(editorHwnd) != nullptr;
        AdjustWindowRectEx(&rc, style, fMenu, exStyle);
        MoveWindow(editorHwnd, 0, 0, rc.right - rc.left, rc.bottom - rc.top, TRUE);
    }
}

LRESULT CALLBACK VstWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto vstPlugin = reinterpret_cast<VstPlugin*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (message) {
    case WM_CREATE: {
        vstPlugin = reinterpret_cast<VstPlugin*>(((LPCREATESTRUCT)lParam)->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)((LPCREATESTRUCT)lParam)->lpCreateParams);
        break;
    }
    case WM_CLOSE: {
        vstPlugin->closingEditorWindow();
        break;
    }
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

void VstPlugin::openEditor(HWND hWndParent)
{
    if (!flagsHasEditor()) {
        return;
    }

    if (editorHwnd == nullptr) {
        WNDCLASSEX wcex{ sizeof(wcex) };
        wcex.lpfnWndProc = VstWindowProc;
        wcex.hInstance = GetModuleHandle(0);
        wcex.lpszClassName = L"Minimal VST host - Guest VST Window Frame";
        RegisterClassEx(&wcex);

        const auto style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
        editorHwnd = CreateWindow(
            wcex.lpszClassName, L"VST Plugin", style,
            0, 0, 0, 0, hWndParent, 0, 0,
            reinterpret_cast<LPVOID>(this));
        dispatcher(effEditOpen, 0, 0, editorHwnd);
        RECT rc{};
        ERect* erc = nullptr;
        dispatcher(effEditGetRect, 0, 0, &erc);
        rc.left = erc->left;
        rc.top = erc->top;
        rc.right = erc->right;
        rc.bottom = erc->bottom;
        resizeEditor(rc);
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
    if (auto l = vstMidi.lock()) {
        vstMidi.events.push_back(e);
    }
}

// This function is called from refillCallback() which is running in audio thread.
void VstPlugin::processEvents()
{
    vstMidiEvents.clear();
    if (auto l = vstMidi.lock()) {
        std::swap(vstMidiEvents, vstMidi.events);
    }
    if (!vstMidiEvents.empty()) {
        const auto n = vstMidiEvents.size();
        const auto bytes = sizeof(VstEvents) + sizeof(VstEvent*) * n;
        vstEventBuffer.resize(bytes);
        auto* ve = reinterpret_cast<VstEvents*>(vstEventBuffer.data());
        ve->numEvents = n;
        ve->reserved = 0;
        for (size_t i = 0; i < n; ++i) {
            ve->events[i] = reinterpret_cast<VstEvent*>(&vstMidiEvents[i]);
        }
        dispatcher(effProcessEvents, 0, 0, ve);
    }
}

// This function is called from refillCallback() which is running in audio thread.
float** VstPlugin::processAudio(size_t frameCount, size_t& outputFrameCount)
{
    frameCount = std::min<size_t>(frameCount, outputBuffer.size() / getChannelCount());
    aEffect->processReplacing(aEffect, inputBufferHeads.data(), outputBufferHeads.data(), frameCount);
    samplePos += frameCount;
    outputFrameCount = frameCount;
    return outputBufferHeads.data();
}

bool VstPlugin::init(const wchar_t* vstModulePath, HWND hWndParent)
{
    {
        wchar_t buf[MAX_PATH + 1]{};
        wchar_t* namePtr = nullptr;
        const auto r = GetFullPathName(vstModulePath, _countof(buf), buf, &namePtr);
        if (r && namePtr) {
            *namePtr = 0;
            char mbBuf[_countof(buf) * 4]{};
            if (auto s = WideCharToMultiByte(CP_OEMCP, 0, buf, -1, mbBuf, sizeof(mbBuf), 0, 0)) {
                directoryMultiByte = mbBuf;
            }
        }
    }

    hModule = LoadLibrary(vstModulePath);
    ASSERT_THROW(hModule, "Can't open VST DLL");

    typedef AEffect*(VstEntryProc)(audioMasterCallback);
    auto* vstEntryProc = reinterpret_cast<VstEntryProc*>(GetProcAddress(hModule, "VSTPluginMain"));
    if (!vstEntryProc) {
        vstEntryProc = reinterpret_cast<VstEntryProc*>(GetProcAddress(hModule, "main"));
    }
    ASSERT_THROW(vstEntryProc, "VST's entry point not found");

    aEffect = vstEntryProc(hostCallback_static);
    ASSERT_THROW(aEffect && aEffect->magic == kEffectMagic, "Not a VST plugin");
    ASSERT_THROW(flagsIsSynth(), "Not a VST Synth");
    aEffect->user = this;

    inputBuffer.resize(aEffect->numInputs * getBlockSize());
    for (int i = 0; i < aEffect->numInputs; ++i) {
        inputBufferHeads.push_back(&inputBuffer[i * getBlockSize()]);
    }

    outputBuffer.resize(aEffect->numOutputs * getBlockSize());
    for (int i = 0; i < aEffect->numOutputs; ++i) {
        outputBufferHeads.push_back(&outputBuffer[i * getBlockSize()]);
    }

    dispatcher(effOpen);
    dispatcher(effSetSampleRate, 0, 0, 0, static_cast<float>(getSampleRate()));
    dispatcher(effSetBlockSize, 0, getBlockSize());
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
    if (editorHwnd) {
        dispatcher(effEditClose);
        editorHwnd = nullptr;
    }
    dispatcher(effStopProcess);
    dispatcher(effMainsChanged, 0, 0);
    dispatcher(effClose);
    if (hModule) {
        FreeLibrary(hModule);
        hModule = nullptr;
    }
}

VstIntPtr VstPlugin::hostCallback_static(AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
{
    if (effect && effect->user) {
        auto* that = static_cast<VstPlugin*>(effect->user);
        return that->hostCallback(opcode, index, value, ptr, opt);
    }

    switch (opcode) {
    case audioMasterVersion:
        return kVstVersion;
    default:
        return 0;
    }
}

VstIntPtr VstPlugin::hostCallback(VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float)
{
    switch (opcode) {
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
        strcpy_s(static_cast<char*>(ptr), kVstMaxVendorStrLen, getVendorString());
        return 1;

    case audioMasterGetProductString:
        strcpy_s(static_cast<char*>(ptr), kVstMaxProductStrLen, getProductString());
        return 1;

    case audioMasterGetTime:
        timeinfo.flags = 0;
        timeinfo.samplePos = getSamplePos();
        timeinfo.sampleRate = getSampleRate();
        return reinterpret_cast<VstIntPtr>(&timeinfo);

    case audioMasterGetDirectory:
        return reinterpret_cast<VstIntPtr>(directoryMultiByte.c_str());

    case audioMasterIdle:
        if (editorHwnd) {
            dispatcher(effEditIdle);
        }
        break;

    case audioMasterSizeWindow:
        if (editorHwnd) {
            RECT rc{};
            GetWindowRect(editorHwnd, &rc);
            rc.right = rc.left + static_cast<int>(index);
            rc.bottom = rc.top + static_cast<int>(value);
            resizeEditor(rc);
        }
        break;

    case audioMasterCanDo:
        for (const char** pp = getCapabilities(); *pp; ++pp) {
            if (strcmp(*pp, static_cast<const char*>(ptr)) == 0) {
                return 1;
            }
        }
        return 0;
    }
    return 0;
}
