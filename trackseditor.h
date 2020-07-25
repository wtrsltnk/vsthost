#ifndef TRACKSEDITOR_H
#define TRACKSEDITOR_H

#include "state.h"
#include "tracksmanager.h"

#include <chrono>
#include <imgui.h>

class TracksEditor
{
    State *_state = nullptr;
    TracksManager *_tracks = nullptr;
    int _pixelsPerStep = 20;

    int _trackHeight = 200;
    int _editTrackName = -1;
    char _editTrackBuffer[128] = {0};

    ImColor _trackbgcol = ImColor(55, 55, 55, 55);
    ImColor _trackaltbgcol = ImColor(0, 0, 0, 0);
    ImColor _trackactivebgcol = ImColor(55, 88, 155, 55);

    float TimeToPixels(
        std::chrono::milliseconds::rep time);

    float StepsToPixels(
        long steps);

    std::chrono::milliseconds::rep PixelsToTime(
        float pixels);

    long PixelsToSteps(
        float pixels);

    int MaxTracksWidth();

    void RenderCursor(
        ImVec2 const &p);

    void RenderGrid(
        ImVec2 const &p,
        int trackWidth);

    void RenderTimeline(
        ImVec2 const &p,
        int trackWidth);

    void RenderTrackHeader(
        Track *track,
        int t,
        int trackWidth);

    void RenderTrack(
        Track *track,
        int t,
        int trackWidth);

public:
    TracksEditor();

    int EditTrackName() const;
    void SetState(State *state);
    void SetTracksManager(TracksManager *tracks);

    void Render(
        ImVec2 const &pos,
        ImVec2 const &size);
};

#endif // TRACKSEDITOR_H
