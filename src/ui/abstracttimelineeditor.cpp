#include "abstracttimelineeditor.h"

#include <algorithm>
#include <sstream>

const ImColor color = ImColor(55, 55, 55, 55);
const ImColor accentColor = ImColor(55, 55, 55, 155);
const ImColor timelineTextColor = ImColor(155, 155, 155, 255);

AbstractTimelineEditor::AbstractTimelineEditor()
{
}

void AbstractTimelineEditor::SetState(
    State *state)
{
    _state = state;
}

void AbstractTimelineEditor::SetTracksManager(
    ITracksManager *tracks)
{
    _state->_tracks = tracks;
}

std::chrono::milliseconds::rep AbstractTimelineEditor::PixelsToSteps(
    float pixels)
{
    return std::chrono::milliseconds::rep((pixels / _pixelsPerStep) * 1000.0f);
}

float AbstractTimelineEditor::StepsToPixels(
    std::chrono::milliseconds::rep steps)
{
    return (steps / 1000.0f) * _pixelsPerStep;
}

long AbstractTimelineEditor::SnapRegionsSteps(
    long steps)
{
    return steps - (steps % _snapRegionsToSteps);
}

long AbstractTimelineEditor::SnapNotesSteps(
    long steps)
{
    return steps - (steps % _snapNotesToSteps);
}

void AbstractTimelineEditor::RenderGrid(
    ImVec2 const &p,
    float windowWidth,
    float fullTracksHeight)
{
    int step = 0;
    int gg = 0;

    while (gg < std::max(windowWidth, ImGui::GetContentRegionAvail().x))
    {
        auto cursor = ImVec2(p.x + StepsToPixels(step), p.y);

        ImGui::GetWindowDrawList()->AddLine(
            cursor,
            ImVec2(cursor.x, cursor.y + fullTracksHeight - ImGui::GetStyle().ItemSpacing.y),
            gg % (_pixelsPerStep * 4) == 0 ? accentColor : color,
            1.0f);

        step += 1000;
        gg += _pixelsPerStep;
    }
}

void AbstractTimelineEditor::RenderCursor(
    ImVec2 const &p,
    ImVec2 const &size,
    int scrollX,
    int horizontalOffset)
{
    auto cursor = ImVec2(p.x + StepsToPixels(_state->_cursor) - float(scrollX), p.y);

    if (cursor.x < ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMin().x + horizontalOffset)
    {
        return;
    }

    ImGui::GetWindowDrawList()->AddLine(
        cursor,
        ImVec2(cursor.x, cursor.y + size.y - trackToolsHeight - trackToolsHeight),
        ImGui::GetColorU32(ImGui::GetStyle().Colors[ImGuiCol_PlotHistogramHovered]),
        3.0f);
}

void AbstractTimelineEditor::RenderTimeline(
    ImVec2 const &screenOrigin,
    int windowWidth,
    int scrollX)
{
    ImGui::SetCursorScreenPos(screenOrigin);

    if (ImGui::InvisibleButton("##TimeLine", ImVec2(float(windowWidth), float(timelineHeight))))
    {
        int step = int(float((ImGui::GetMousePos().x - screenOrigin.x + scrollX) / _pixelsPerStep) + 0.5f);
        _state->SetCursorAtStep(step);
    }

    int step = 1;
    for (int g = 0; g < std::max(windowWidth, int(ImGui::GetContentRegionAvail().x)); g += (_pixelsPerStep * 4))
    {
        std::stringstream ss;
        ss << (step++);
        auto cursor = ImVec2(screenOrigin.x - scrollX + g, screenOrigin.y);
        if (cursor.x >= screenOrigin.x)
        {
            ImGui::GetWindowDrawList()->AddLine(
                ImVec2(cursor.x, cursor.y),
                ImVec2(cursor.x, cursor.y + timelineHeight),
                accentColor);

            ImGui::GetWindowDrawList()->AddText(
                ImVec2(cursor.x + 5, cursor.y),
                timelineTextColor,
                ss.str().c_str());
        }
    }

    auto cursor = ImVec2(screenOrigin.x - scrollX + windowWidth, screenOrigin.y);
    if (cursor.x >= screenOrigin.x)
    {
        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(cursor.x, cursor.y),
            ImVec2(cursor.x, cursor.y + timelineHeight),
            accentColor);
    }
}
