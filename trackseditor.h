#ifndef TRACKSEDITOR_H
#define TRACKSEDITOR_H

#include "itrack.h"
#include "itracksmanager.h"
#include "region.h"
#include "state.h"

#include <chrono>
#include <imgui.h>
#include <map>
#include <string>
#include <vector>

class TracksEditor
{
    State *_state = nullptr;
    ITracksManager *_tracks = nullptr;
    int _pixelsPerStep = 20;
    int _snapRegionsToSteps = 1000;
    int _snapNotesToSteps = 100;

    int _trackHeight = 200;
    int _editTrackName = -1;
    char _editTrackBuffer[128] = {0};
    int _maxTrackLength = 16000;

    ImColor _trackbgcol = ImColor(55, 55, 55, 55);
    ImColor _trackaltbgcol = ImColor(0, 0, 0, 0);
    ImColor _trackactivebgcol = ImColor(55, 88, 155, 55);

    ImVec2 _mouseDragStart;
    ITrack *_mouseDragTrack = nullptr;

    long _mouseDragFrom = -1, moveTo = -1;
    bool _doMove = false;

    void HandleTracksEditorShortCuts();

    void FinishDragRegion(
        long newX);

    void StartDragRegion(
        ITrack *track,
        std::pair<long, Region> region);

    long GetNewRegionStart(
        std::pair<long, Region> region);

    long GetNewRegionLength(
        std::pair<long, Region> region);

    float StepsToPixels(
        long steps);

    long PixelsToSteps(
        float pixels);

    long SnapRegionsSteps(
        long steps);

    long SnapNotesSteps(
        long steps);

    int MaxTracksWidth();

    void UpdateRegionLength(
        ITrack *track,
        long regionAt,
        long length);

    void RenderCursor(
        ImVec2 const &p,
        ImVec2 const &size,
        int scrollX);

    void RenderGrid(
        ImVec2 const &p,
        int trackWidth,
        int fullTracksHeight);

    void RenderTimeline(
        ImVec2 const &p,
        int trackWidth,
        int scrollX);

    void RenderTrackHeader(
        ITrack *track,
        int t);

    void RenderTrack(
        ITrack *track,
        int t,
        int trackWidth);

    void RenderRegion(
        ITrack *track,
        std::pair<const long, Region> const &region,
        ImVec2 const &trackOrigin,
        ImVec2 const &trackScreenOrigin,
        int finalTrackHeight);

    void RenderNotes(
        std::pair<const long, Region> const &region,
        ImVec2 const &trackScreenOrigin,
        int finalTrackHeight);

public:
    TracksEditor();

    int EditTrackName() const;

    void SetState(
        State *state);

    void SetTracksManager(
        ITracksManager *tracks);

    void Render(
        ImVec2 const &pos,
        ImVec2 const &size);
};

#endif // TRACKSEDITOR_H
