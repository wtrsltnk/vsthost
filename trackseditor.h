#ifndef TRACKSEDITOR_H
#define TRACKSEDITOR_H

#include "abstracttimelineeditor.h"

#include <chrono>
#include <imgui.h>
#include <map>
#include <string>
#include <vector>

class TracksEditor :
    public AbstractTimelineEditor
{
    int _trackHeight = 200;
    int _editTrackName = -1;
    char _editTrackBuffer[128] = {0};
    std::chrono::milliseconds::rep _maxTrackLength = 16000;

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

    void StartRegionResize(
        ITrack *track,
        std::pair<long, Region> region);

    void ResizeRegion(
        ITrack *track,
        std::pair<long, Region> region);

    void CreateRegion(
        ITrack *track,
        const ImVec2 &pp);

    void MoveRegion(
        ITrack *track);

    long GetNewRegionStart(
        std::pair<long, Region> region);

    long GetNewRegionLength(
        std::pair<long, Region> region);

    std::chrono::milliseconds::rep MaxTracksWidth();

    void UpdateRegionLength(
        ITrack *track,
        long regionAt,
        long length);

    void RenderTrackHeader(
        ITrack *track,
        int t);

    void RenderTrack(
        ITrack *track,
        int t,
        float trackWidth);

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

    void Render(
        ImVec2 const &pos,
        ImVec2 const &size);
};

#endif // TRACKSEDITOR_H
