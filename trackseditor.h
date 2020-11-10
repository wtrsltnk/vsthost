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
    Track *_mouseDragTrack = nullptr;

    long _mouseDragFrom = -1, moveTo = -1;
    bool _doMove = false;

    void HandleTracksEditorShortCuts();

    void FinishDragRegion(
        long newX);

    void StartRegionResize(
        Track &track,
        std::pair<long, Region> region);

    void ResizeRegion(
        Track &track,
        std::pair<long, Region> region);

    void CreateRegion(
        Track &track,
        const ImVec2 &pp);

    void MoveRegion(
        Track &track);

    long GetNewRegionStart(
        std::pair<long, Region> region);

    long GetNewRegionLength(
        std::pair<long, Region> region);

    std::chrono::milliseconds::rep MaxTracksWidth();

    void UpdateRegionLength(
        Track &track,
        long regionAt,
        long length);

    void RenderTrackHeader(
        Track &track,
        int t);

    void RenderTrack(
        Track &track,
        int t,
        float trackWidth);

    void RenderRegion(
        Track &track,
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
