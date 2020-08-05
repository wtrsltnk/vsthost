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
    int _snapToPixels = 1000;

    int _trackHeight = 200;
    int _editTrackName = -1;
    char _editTrackBuffer[128] = {0};
    int _maxTrackLength = 16000;

    ImColor _trackbgcol = ImColor(55, 55, 55, 55);
    ImColor _trackaltbgcol = ImColor(0, 0, 0, 0);
    ImColor _trackactivebgcol = ImColor(55, 88, 155, 55);

    ImVec2 _mouseDragStart;
    Track *_mouseDragTrack = nullptr;

    long _mouseDragFrom = -1, moveTo = -1;
    bool _doMove = false;
    bool _zoomInOnActiveRegion = false;
    int _pixelsPerStepBeforeZoom = 20;
    int _scrollXOnNextFrame = -1;

    long ZoomIn(
        Track *track,
        std::pair<const long, Region> const &region);

    void ZoomOut();

    void HandleTracksEditorShortCuts();

    void FinishDragRegion(
        long newX);

    void StartDragRegion(
        Track *track,
        std::pair<long, Region> region);

    long GetNewRegionStart(
        std::pair<long, Region> region);

    long GetNewRegionLength(
        std::pair<long, Region> region);

    float StepsToPixels(
        long steps);

    long PixelsToSteps(
        float pixels);

    int MaxTracksWidth();

    void UpdateRegionLength(
        Track *track,
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
        Track *track,
        int t);

    void RenderTrack(
        Track *track,
        int t,
        int trackWidth);

    void RenderRegion(
        Track *track,
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
    void SetState(State *state);
    void SetTracksManager(TracksManager *tracks);

    void Render(
        ImVec2 const &pos,
        ImVec2 const &size);
};

#endif // TRACKSEDITOR_H
