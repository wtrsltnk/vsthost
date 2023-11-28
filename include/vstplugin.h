#ifndef VSTPLUGIN_H
#define VSTPLUGIN_H

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <windows.h>

#pragma warning(push)
#pragma warning(disable : 4996)
#include <pluginterfaces/vst2.x/aeffectx.h>
#include <public.sdk/source/vst2.x/audioeffect.h>
#pragma warning(pop)

class VstPlugin
{
public:
    VstPlugin();

    ~VstPlugin();

    const char *Title() const;

    const char *Vendor() const;

    std::string ModulePath() const;

    AEffect *getEffect();

    size_t getSamplePos() const;

    size_t getSampleRate() const;

    size_t getBlockSize() const;

    size_t getChannelCount() const;

    std::string const &getEffectName() const;

    std::string const &getVendorName() const;

    int getProgramCount() const;

    int getParamCount() const;

    int getInputCount() const;

    int getOutputCount() const;

    bool getFlags(
        int32_t m) const;

    bool flagsHasEditor() const;

    bool flagsIsSynth() const;

    intptr_t dispatcher(
        int32_t opcode,
        int32_t index = 0,
        intptr_t value = 0,
        void *ptr = nullptr,
        float opt = 0.0f) const;

    void resizeEditor(
        const RECT &clientRc) const;

    void openEditor(
        HWND hWndParent);

    bool isEditorOpen();

    void closeEditor();

    void sendMidiNote(
        int midiChannel,
        int noteNumber,
        bool onOff,
        int velocity);

    // This function is called from refillCallback() which is running in audio thread.
    void processEvents();

    // This function is called from refillCallback() which is running in audio thread.
    float **processAudio(
        size_t frameCount,
        size_t &outputFrameCount);

    bool init(
        const char *vstModulePath);

    void cleanup();

    static const char *getVendorString();

    static const char *getProductString();

    static int getVendorVersion();

    static const char **getCapabilities();

    std::vector<float *> _inputBufferHeads;

private:
    static VstIntPtr hostCallback_static(
        AEffect *effect,
        VstInt32 opcode,
        VstInt32 index,
        VstIntPtr value,
        void *ptr,
        float opt);

    VstIntPtr hostCallback(
        VstInt32 opcode,
        VstInt32 index,
        VstIntPtr value,
        void *ptr,
        float);

protected:
    std::wstring _modulePath;
    std::string _moduleDirectory;

    HWND _editorHwnd = nullptr;
    HMODULE _vstLibraryHandle = nullptr;
    AEffect *_aEffect = nullptr;
    std::atomic<size_t> _samplePos;
    VstTimeInfo _timeinfo;

    std::vector<float> _outputBuffer;
    std::vector<float *> _outputBufferHeads;
    std::vector<float> _inputBuffer;

    std::vector<VstMidiEvent> _vstMidiEvents;
    std::vector<char> _vstEventBuffer;
    std::string _vstEffectName;
    std::string _vstVendorName;

    struct
    {
        std::vector<VstMidiEvent> events;
        std::unique_lock<std::mutex> lock() const
        {
            return std::unique_lock<std::mutex>(mutex);
        }

    private:
        std::mutex mutable mutex;
    } _vstMidi;

    friend LRESULT CALLBACK VstWindowProc(
        HWND hwnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam);

    void closingEditorWindow();
};

#endif // VSTPLUGIN_H
