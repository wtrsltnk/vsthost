#ifndef INSPECTORWINDOW_H
#define INSPECTORWINDOW_H

#include "itracksmanager.h"
#include "state.h"

#include <RtMidi.h>
#include <imgui.h>

class InspectorWindow
{
    State *_state = nullptr;
    ITracksManager *_tracks = nullptr;
    RtMidiIn *_midiIn = nullptr;

public:
    InspectorWindow();

    void SetState(
        State *state);

    void SetTracksManager(
        ITracksManager *tracks);

    void SetMidiIn(
        RtMidiIn *midiIn);

    void Render(
        ImVec2 const &pos,
        ImVec2 const &size);
};

#endif // INSPECTORWINDOW_H
