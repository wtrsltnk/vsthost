#ifndef VSTPLUGIN_H
#define VSTPLUGIN_H

#include <windows.h>
#include <atomic>
#include <string>
#include <vector>
#include <memory>
#include <mutex>

#pragma warning(push)
#pragma warning(disable : 4996)
#include <pluginterfaces/vst2.x/aeffectx.h>
#include <public.sdk/source/vst2.x/audioeffect.h>
#pragma warning(pop)

class VstPlugin {
public:
    VstPlugin(const wchar_t* vstModulePath, HWND hWndParent);

    ~VstPlugin();

    size_t  getSamplePos()      const { return samplePos; }
    size_t  getSampleRate()     const { return 44100; }
    size_t  getBlockSize()      const { return 1024; }
    size_t  getChannelCount()   const { return 2; }
    std::string const &getEffectName()    const { return vstEffectName; }
    std::string const &getVendorName()    const { return vstVendorName; }
    static const char*  getVendorString() { return "TEST_VENDOR"; }
    static const char*  getProductString() { return "TEST_PRODUCT"; }
    static int          getVendorVersion() { return 1; }
    static const char** getCapabilities() {
        static const char* hostCapabilities[] = {
            "sendVstEvents",
            "sendVstMidiEvents",
            "sizeWindow",
            "startStopProcess",
            "sendVstMidiEventFlagIsRealtime",
            nullptr
        };
        return hostCapabilities;
    }

    bool    getFlags(int32_t m) const { return (aEffect->flags & m) == m; }
    bool    flagsHasEditor()    const { return getFlags(effFlagsHasEditor); }
    bool    flagsIsSynth()      const { return getFlags(effFlagsIsSynth); }
    intptr_t dispatcher(int32_t opcode, int32_t index = 0, intptr_t value = 0, void *ptr = nullptr, float opt = 0.0f) const;

    void resizeEditor(const RECT& clientRc) const;
    void openEditor(HWND hWndParent);
    bool isEditorOpen();
    void closeEditor();

    void sendMidiNote(int midiChannel, int noteNumber, bool onOff, int velocity);

    // This function is called from refillCallback() which is running in audio thread.
    void processEvents();

    // This function is called from refillCallback() which is running in audio thread.
    float** processAudio(size_t frameCount, size_t& outputFrameCount);

private:
    bool init(const wchar_t* vstModulePath, HWND hWndParent);
    void cleanup();
    static VstIntPtr hostCallback_static(AEffect* effect, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt);
    VstIntPtr hostCallback(VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float);

protected:
    HWND                editorHwnd { nullptr };
    HMODULE             hModule { nullptr };
    AEffect*            aEffect { nullptr };
    std::atomic<size_t> samplePos { 0 };
    VstTimeInfo         timeinfo {};
    std::string         directoryMultiByte {};

    std::vector<float>          outputBuffer;
    std::vector<float*>         outputBufferHeads;
    std::vector<float>          inputBuffer;
    std::vector<float*>         inputBufferHeads;

    std::vector<VstMidiEvent>   vstMidiEvents;
    std::vector<char>           vstEventBuffer;
    std::string                 vstEffectName;
    std::string                 vstVendorName;

    struct {
        std::vector<VstMidiEvent>   events;
        std::unique_lock<std::mutex> lock() const {
            return std::unique_lock<std::mutex>(mutex);
        }
    private:
        std::mutex mutable          mutex;
    } vstMidi;

    friend LRESULT CALLBACK VstWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    void closingEditorWindow();

};

#endif // VSTPLUGIN_H
