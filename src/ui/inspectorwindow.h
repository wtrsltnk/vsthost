#ifndef INSPECTORWINDOW_H
#define INSPECTORWINDOW_H

#include "../state.h"
#include "../wasapi.h"
#include <ipluginservice.h>
#include <itracksmanager.h>

#include <RtMidi.h>
#include <imgui.h>

class InspectorWindow
{
public:
    void SetState(
        State *state);

    void SetTracksManager(
        ITracksManager *tracks);

    void SetMidiIn(
        RtMidiIn *midiIn);

    void SetAudioOut(
        struct Wasapi *wasapi);

    void SetVstPluginLoader(
        IPluginService *loader);

    void Render(
        ImVec2 const &pos,
        ImVec2 const &size);

private:
    State *_state = nullptr;
    ITracksManager *_tracks = nullptr;
    RtMidiIn *_midiIn = nullptr;
    struct Wasapi *_wasapi = nullptr;
    IPluginService *_vstPluginLoader = nullptr;
    bool _editRegionName = false;
    char _editRegionNameBuffer[128] = {0};

    std::vector<struct PluginDescription> PluginLibrary(
        const char *id,
        std::function<void(const std::shared_ptr<class VstPlugin> &)> onPLuginSelected,
        std::vector<struct PluginDescription> &plugins,
        std::function<bool(const struct PluginDescription &)> filter);
};

#endif // INSPECTORWINDOW_H
