#include <vstplugin.h>

#include <filesystem>
#include <iostream>
#include <stdexcept>

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

VstPlugin::VstPlugin() = default;

VstPlugin::~VstPlugin()
{
    if (_vstLibraryHandle != NULL)
    {
        cleanup();
    }
}

const char *VstPlugin::Title() const
{
    return _vstEffectName.c_str();
}

const char *VstPlugin::Vendor() const
{
    return _vstVendorName.c_str();
}

std::string VstPlugin::ModulePath() const
{
    auto fullPath = std::filesystem::path(_moduleDirectory) / std::filesystem::path(_modulePath);

    return fullPath.string().c_str();
}

AEffect *VstPlugin::getEffect()
{
    return _aEffect;
}

size_t VstPlugin::getSamplePos() const
{
    return _samplePos;
}

size_t VstPlugin::getSampleRate() const
{
    return 44100;
}

size_t VstPlugin::getBlockSize() const
{
    return 1024;
}

size_t VstPlugin::getChannelCount() const
{
    return 2;
}

std::string const &VstPlugin::getEffectName() const
{
    return _vstEffectName;
}

std::string const &VstPlugin::getVendorName() const
{
    return _vstVendorName;
}

const char *VstPlugin::getVendorString()
{
    return "TEST_VENDOR";
}

const char *VstPlugin::getProductString()
{
    return "TEST_PRODUCT";
}

int VstPlugin::getVendorVersion()
{
    return 1;
}

bool VstPlugin::getFlags(
    int32_t m) const
{
    return (_aEffect->flags & m) == m;
}

bool VstPlugin::flagsHasEditor() const
{
    return getFlags(effFlagsHasEditor);
}

bool VstPlugin::flagsIsSynth() const
{
    return getFlags(effFlagsIsSynth);
}

static const char *hostCapabilities[] = {
    "sendVstEvents",
    "sendVstMidiEvents",
    "sizeWindow",
    "startStopProcess",
    "sendVstMidiEventFlagIsRealtime",
    nullptr,
};

const char **VstPlugin::getCapabilities()
{
    return hostCapabilities;
}

intptr_t VstPlugin::dispatcher(
    int32_t opcode,
    int32_t index,
    intptr_t value,
    void *ptr,
    float opt) const
{
    return _aEffect->dispatcher(_aEffect, opcode, index, value, ptr, opt);
}

void VstPlugin::resizeEditor(
    const RECT &clientRc) const
{
    if (_editorHwnd == nullptr)
    {
        return;
    }

    auto rc = clientRc;
    DWORD style = static_cast<DWORD>(GetWindowLongPtr(_editorHwnd, GWL_STYLE));
    DWORD exStyle = static_cast<DWORD>(GetWindowLongPtr(_editorHwnd, GWL_EXSTYLE));

    AdjustWindowRectEx(&rc, style, GetMenu(_editorHwnd) != nullptr, exStyle);
    MoveWindow(_editorHwnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
}

LRESULT CALLBACK VstWindowProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    auto vstPlugin = reinterpret_cast<VstPlugin *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (message)
    {
        case WM_CREATE:
        {
            auto createStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
            break;
        }
        case WM_KEYDOWN:
        {
            if (wParam == VK_ESCAPE)
            {
                vstPlugin->closingEditorWindow();
                DestroyWindow(hwnd);
            }
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

void VstPlugin::openEditor(
    HWND hWndParent)
{
    if (!flagsHasEditor())
    {
        return;
    }

    if (_editorHwnd == nullptr)
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
        wcex.lpszClassName = "Minimal VST host - Guest VST Window Frame";
        wcex.lpszMenuName = nullptr;
        RegisterClassEx(&wcex);

        auto xscreen = GetSystemMetrics(SM_CXSCREEN);
        auto yscreen = GetSystemMetrics(SM_CYSCREEN);

        _editorHwnd = CreateWindow(
            wcex.lpszClassName,
            "VST Plugin",
            WS_OVERLAPPED | WS_CAPTION,
            CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT,
            hWndParent, nullptr, nullptr,
            reinterpret_cast<LPVOID>(this));

        dispatcher(effEditOpen, 0, 0, _editorHwnd);

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

    ShowWindow(_editorHwnd, SW_SHOW);
}
bool VstPlugin::isEditorOpen()
{
    return _editorHwnd != nullptr;
}

void VstPlugin::closeEditor()
{
    HWND tmp = _editorHwnd;
    closingEditorWindow();
    DestroyWindow(tmp);
}

void VstPlugin::closingEditorWindow()
{
    dispatcher(effEditClose, 0, 0, _editorHwnd);
    _editorHwnd = nullptr;
}

void VstPlugin::sendMidiNote(
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
    if (auto l = _vstMidi.lock())
    {
        (void)l;
        _vstMidi.events.push_back(e);
    }
}

// This function is called from refillCallback() which is running in audio thread.
void VstPlugin::processEvents()
{
    _vstMidiEvents.clear();

    if (auto l = _vstMidi.lock())
    {
        (void)l;
        std::swap(_vstMidiEvents, _vstMidi.events);
    }

    if (!_vstMidiEvents.empty())
    {
        const auto n = _vstMidiEvents.size();
        const auto bytes = sizeof(VstEvents) + sizeof(VstEvent *) * n;
        _vstEventBuffer.resize(bytes);
        auto *ve = reinterpret_cast<VstEvents *>(_vstEventBuffer.data());
        ve->numEvents = static_cast<int>(n);
        ve->reserved = 0;
        for (size_t i = 0; i < n; ++i)
        {
            ve->events[i] = reinterpret_cast<VstEvent *>(&_vstMidiEvents[i]);
        }
        dispatcher(effProcessEvents, 0, 0, ve);
    }
}

// This function is called from refillCallback() which is running in audio thread.
float **VstPlugin::processAudio(
    size_t frameCount,
    size_t &outputFrameCount)
{
    frameCount = std::min<size_t>(frameCount, _outputBuffer.size() / getChannelCount());

    _aEffect->processReplacing(_aEffect, _inputBufferHeads.data(), _outputBufferHeads.data(), static_cast<int>(frameCount));
    _samplePos += frameCount;
    outputFrameCount = frameCount;

    return _outputBufferHeads.data();
}

bool VstPlugin::init(
    const char *vstModulePath)
{
    _modulePath = vstModulePath;

    {
        char buf[MAX_PATH + 1];
        char *namePtr = nullptr;
        const auto r = GetFullPathName(vstModulePath, _countof(buf), buf, &namePtr);
        if (r && namePtr)
        {
            *namePtr = 0;
            _moduleDirectory = buf;
        }
    }

    _vstLibraryHandle = LoadLibrary(vstModulePath);
    ASSERT_THROW(_vstLibraryHandle, "Can't open VST DLL")

    typedef AEffect *(VstEntryProc)(audioMasterCallback);
    auto *vstEntryProc = reinterpret_cast<VstEntryProc *>(GetProcAddress(_vstLibraryHandle, "VSTPluginMain"));
    if (!vstEntryProc)
    {
        vstEntryProc = reinterpret_cast<VstEntryProc *>(GetProcAddress(_vstLibraryHandle, "main"));
    }
    ASSERT_THROW(vstEntryProc, "VST's entry point not found")

    _aEffect = vstEntryProc(hostCallback_static);
    ASSERT_THROW(_aEffect && _aEffect->magic == kEffectMagic, "Not a VST plugin")
    ASSERT_THROW(flagsIsSynth(), "Not a VST Synth")
    _aEffect->user = this;

    _inputBuffer.resize(static_cast<size_t>(_aEffect->numInputs) * getBlockSize());
    for (int i = 0; i < _aEffect->numInputs; ++i)
    {
        _inputBufferHeads.push_back(&_inputBuffer[static_cast<size_t>(i) * getBlockSize()]);
    }

    _outputBuffer.resize(static_cast<size_t>(_aEffect->numOutputs) * getBlockSize());
    for (int i = 0; i < _aEffect->numOutputs; ++i)
    {
        _outputBufferHeads.push_back(&_outputBuffer[static_cast<size_t>(i) * getBlockSize()]);
    }

    dispatcher(effOpen);
    dispatcher(effSetSampleRate, 0, 0, nullptr, static_cast<float>(getSampleRate()));
    dispatcher(effSetBlockSize, 0, static_cast<intptr_t>(getBlockSize()));
    dispatcher(effSetProcessPrecision, 0, kVstProcessPrecision32);
    dispatcher(effMainsChanged, 0, 1);
    dispatcher(effStartProcess);

    char ceffname[kVstMaxEffectNameLen] = "";
    dispatcher(effGetEffectName, 0, 0, ceffname, 0.0f);
    _vstEffectName = ceffname;

    char cvendor[kVstMaxVendorStrLen] = "";
    dispatcher(effGetVendorString, 0, 0, cvendor, 0.0f);
    _vstVendorName = cvendor;

    return true;
}

void VstPlugin::cleanup()
{
    if (_editorHwnd)
    {
        dispatcher(effEditClose);
        _editorHwnd = nullptr;
    }
    //dispatcher(effStopProcess);
    // dispatcher(effMainsChanged, 0, 0);
    //dispatcher(effClose);
    if (_vstLibraryHandle)
    {
        FreeLibrary(_vstLibraryHandle);
        _vstLibraryHandle = nullptr;
    }
}

VstIntPtr VstPlugin::hostCallback_static(
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
        {
            return kVstVersion;
        }
        default:
        {
            return 0;
        }
    }
}

VstIntPtr VstPlugin::hostCallback(
    VstInt32 opcode,
    VstInt32 index,
    VstIntPtr value,
    void *ptr,
    float)
{
    switch (opcode)
    {
        case audioMasterVersion:
        {
            return kVstVersion;
        }
        case audioMasterCurrentId:
        {
            return _aEffect->uniqueID;
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
            _timeinfo.flags = 0;
            _timeinfo.samplePos = static_cast<double>(getSamplePos());
            _timeinfo.sampleRate = static_cast<double>(getSampleRate());
            return reinterpret_cast<VstIntPtr>(&_timeinfo);
        }
        case audioMasterGetDirectory:
        {
            return reinterpret_cast<VstIntPtr>(_moduleDirectory.c_str());
        }
        case audioMasterIdle:
        {
            if (_editorHwnd)
            {
                dispatcher(effEditIdle);
            }
            break;
        }
        case audioMasterSizeWindow:
        {
            if (_editorHwnd != nullptr)
            {
                RECT rc;
                GetWindowRect(_editorHwnd, &rc);

                rc.right = rc.left + static_cast<int>(index);
                rc.bottom = rc.top + static_cast<int>(value);

                // resizeEditor(rc);
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
