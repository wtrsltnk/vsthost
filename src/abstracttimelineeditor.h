#ifndef ABSTRACTTIMELINEEDITOR_H
#define ABSTRACTTIMELINEEDITOR_H

#include "state.h"
#include <region.h>
#include <track.h>

#include <imgui.h>

class AbstractTimelineEditor
{
public:
    AbstractTimelineEditor();

    void SetState(
        State *state);

    void SetTracksManager(
        ITracksManager *tracks);

protected:
    State *_state = nullptr;
    int _pixelsPerStep = 20;
    int _snapRegionsToSteps = 1000;
    int _snapNotesToSteps = 100;

    const int trackHeaderWidth = 250;
    const int trackToolsHeight = 30;
    const int timelineHeight = 30;

    float StepsToPixels(
        std::chrono::milliseconds::rep steps);

    std::chrono::milliseconds::rep PixelsToSteps(
        float pixels);

    long SnapRegionsSteps(
        long steps);

    long SnapNotesSteps(
        long steps);

    void RenderCursor(
        ImVec2 const &p,
        ImVec2 const &size,
        int scrollX,
        int horizontalOffset);

    void RenderTimeline(
        ImVec2 const &screenOrigin,
        int windowWidth,
        int scrollX);

    void RenderGrid(
        ImVec2 const &p,
        float windowWidth,
        float fullTracksHeight);
};

#endif // ABSTRACTTIMELINEEDITOR_H
