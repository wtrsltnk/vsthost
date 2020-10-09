#ifndef INSPECTORWINDOW_H
#define INSPECTORWINDOW_H

#include "itracksmanager.h"
#include "ivstpluginloader.h"
#include "state.h"
#include "wasapi.h"

#include <RtMidi.h>
#include <imgui.h>

class InspectorWindow
{
    State *_state = nullptr;
    ITracksManager *_tracks = nullptr;
    RtMidiIn *_midiIn = nullptr;
    struct Wasapi *_wasapi = nullptr;
    IVstPluginLoader *_vstPluginLoader = nullptr;

public:
    InspectorWindow();

    void SetState(
        State *state);

    void SetTracksManager(
        ITracksManager *tracks);

    void SetMidiIn(
        RtMidiIn *midiIn);

    void SetAudioout(
        struct Wasapi *wasapi);

    void SetVstPluginLoader(
        IVstPluginLoader *loader);

    void Render(
        ImVec2 const &pos,
        ImVec2 const &size);
};

#endif // INSPECTORWINDOW_H
