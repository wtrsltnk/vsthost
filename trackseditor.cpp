#include "trackseditor.h"

#include "IconsFontaudio.h"
#include "IconsForkAwesome.h"
#include "imguiutils.h"
#include "midinote.h"

#include <imgui_internal.h>
#include <iostream>
#include <sstream>

const int regionRounding = 5.0f;
const int regionResizeHandleWidth = 20.0f;

TracksEditor::TracksEditor() = default;

int TracksEditor::MaxTracksWidth()
{
    for (auto &track : _tracks->GetTracks())
    {
        for (auto &region : track->Regions())
        {
            auto end = region.first + region.second.Length() + 8000;
            if (end > _maxTrackLength) _maxTrackLength = end;
        }
    }

    return _maxTrackLength;
}

int TracksEditor::EditTrackName() const
{
    return _editTrackName;
}

void TracksEditor::Render(
    const ImVec2 &pos,
    const ImVec2 &size)
{
    if (_pixelsPerStep <= 0) _pixelsPerStep = 1;

    ImGui::Begin(
        "Tracks",
        nullptr,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
    {
        ImGui::SetWindowPos(pos);
        ImGui::SetWindowSize(size);

        auto trackWidth = StepsToPixels(MaxTracksWidth());
        int fullHeight = _tracks->GetTracks().size() * (_trackHeight + ImGui::GetStyle().ItemSpacing.y);

        ImGui::BeginChild(
            "TracksTools",
            ImVec2(0, trackToolsHeight));
        {
            if (ImGui::Button(ICON_FAD_PEN))
            {
                _state->ui._activeCenterScreen = 1;
            }

            ImGui::SameLine();
            ImGui::VerticalSeparator();
            ImGui::SameLine();
            ImGui::PushItemWidth(100);
            ImGui::Text("zoom V :");
            ImGui::SameLine();
            ImGui::SliderInt("##track height", &_trackHeight, 30, 300);
            ImGui::SameLine();

            ImGui::Text("zoom H :");
            ImGui::SameLine();
            ImGui::SliderInt("##zoom", &(_pixelsPerStep), 8, 200);

            ImGui::PopItemWidth();

            ImGui::SameLine();
            ImGui::VerticalSeparator();
            ImGui::SameLine();

            static int e = _snapRegionsToSteps == 4000 ? 0 : 1;
            if (ImGui::RadioButton("Snap to bar", &e, 0))
            {
                _snapRegionsToSteps = 4000;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Snap to step", &e, 1))
            {
                _snapRegionsToSteps = 1000;
            }
            ImGui::SameLine();
        }
        ImGui::EndChild();

        float _tracksScrollx = 0, _tracksScrolly = 0;

        auto contentTop = ImGui::GetCursorPosY();

        ImGui::MoveCursorPos(ImVec2(trackHeaderWidth, 0));

        auto trackScreenOrigin = ImGui::GetCursorScreenPos();

        ImGui::MoveCursorPos(ImVec2(0, timelineHeight));

        ImGui::BeginChild(
            "TracksContainer",
            ImVec2(size.x - trackHeaderWidth - ImGui::GetStyle().ItemSpacing.x, 0),
            false,
            ImGuiWindowFlags_HorizontalScrollbar);
        {
            const ImVec2 p = ImGui::GetCursorScreenPos();

            RenderGrid(p, trackWidth, fullHeight);

            ImGui::BeginGroup();
            {
                int trackIndex = 0;
                for (auto track : _tracks->GetTracks())
                {
                    RenderTrack(
                        track,
                        trackIndex++,
                        trackWidth);
                }
            }
            ImGui::EndGroup();

            _tracksScrollx = ImGui::GetScrollX();
            _tracksScrolly = ImGui::GetScrollY();
        }
        ImGui::EndChild();

        ImGui::SetCursorPosY(contentTop + timelineHeight);
        ImGui::BeginChild(
            "TracksHeadersContainer",
            ImVec2(trackHeaderWidth, 0),
            false,
            ImGuiWindowFlags_NoScrollbar);
        {
            ImGui::SetScrollY(_tracksScrolly);

            ImGui::BeginChild(
                "headers",
                ImVec2(trackHeaderWidth, fullHeight + ImGui::GetStyle().ScrollbarSize),
                false,
                ImGuiWindowFlags_NoScrollbar);
            {
                int trackIndex = 0;
                for (auto track : _tracks->GetTracks())
                {
                    RenderTrackHeader(
                        track,
                        trackIndex++);
                }
            }
            ImGui::EndChild();
        }
        ImGui::EndChild();

        RenderTimeline(
            ImVec2(trackScreenOrigin.x, trackScreenOrigin.y),
            trackWidth,
            _tracksScrollx);

        RenderCursor(trackScreenOrigin, size, _tracksScrollx, trackHeaderWidth);
    }
    ImGui::End();

    HandleTracksEditorShortCuts();
}

void TracksEditor::HandleTracksEditorShortCuts()
{
    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Delete), false))
    {
        _tracks->RemoveActiveRegion();
    }
}

long TracksEditor::GetNewRegionStart(
    std::pair<long, Region> region)
{
    auto diff = ImVec2(ImGui::GetMousePos().x - _mouseDragStart.x, ImGui::GetMousePos().y - _mouseDragStart.y);
    auto newStart = SnapRegionsSteps(region.first + SnapRegionsSteps(PixelsToSteps(diff.x)));

    if (newStart < 0) newStart = 0;

    return newStart;
}

long TracksEditor::GetNewRegionLength(
    std::pair<long, Region> region)
{
    auto diff = ImVec2(ImGui::GetMousePos().x - _mouseDragStart.x, ImGui::GetMousePos().y - _mouseDragStart.y);
    auto snappedDiffX = SnapRegionsSteps(PixelsToSteps(diff.x));
    auto newLength = SnapRegionsSteps(region.second.Length() + snappedDiffX);

    return newLength;
}

void TracksEditor::StartDragRegion(
    ITrack *track,
    std::pair<long, Region> region)
{
    _tracks->SetActiveTrack(track);
    _mouseDragStart = ImGui::GetMousePos();
    _tracks->SetActiveRegion(track, region.first);
}

void TracksEditor::FinishDragRegion(
    long newX)
{
    _doMove = true;
    moveTo = newX;
    _mouseDragStart = ImGui::GetMousePos();
}

void TracksEditor::UpdateRegionLength(
    ITrack *track,
    long regionAt,
    long length)
{
    track->GetRegion(regionAt)
        .SetLength(length);
}

void TracksEditor::RenderRegion(
    ITrack *track,
    std::pair<const long, Region> const &region,
    ImVec2 const &trackOrigin,
    ImVec2 const &trackScreenOrigin,
    int finalTrackHeight)
{
    auto isActiveRegion = std::get<ITrack *>(_tracks->GetActiveRegion()) == track && std::get<long>(_tracks->GetActiveRegion()) == region.first;

    auto regionOrigin = ImVec2(trackOrigin.x + StepsToPixels(region.first), trackOrigin.y + 4);
    auto regionWidth = StepsToPixels(region.second.Length());

    ImGui::PushID(region.first);

    ImGui::SetCursorPos(
        ImVec2(
            regionOrigin.x + regionWidth - regionResizeHandleWidth,
            regionOrigin.y));

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::Button(ICON_FK_ARROWS_H, ImVec2(regionResizeHandleWidth, finalTrackHeight - 8));
    ImGui::PopStyleVar();

    if (ImGui::IsItemClicked(0))
    {
        StartDragRegion(track, region);
    }

    if (ImGui::IsItemActive())
    {
        auto newLength = GetNewRegionLength(region);

        if (newLength > 0 && newLength != region.second.Length())
        {
            UpdateRegionLength(track, region.first, newLength);
            _mouseDragStart = ImGui::GetMousePos();
        }
    }

    ImGui::SetCursorPos(regionOrigin);

    if (isActiveRegion)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(track->GetColor()[0] * 2.0f, track->GetColor()[1] * 2.0f, track->GetColor()[2] * 2.0f, track->GetColor()[3] * 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(track->GetColor()[0] * 2.0f, track->GetColor()[1] * 2.0f, track->GetColor()[2] * 2.0f, track->GetColor()[3] * 0.7f));
    }
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, regionRounding);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::Button("##test", ImVec2(regionWidth, finalTrackHeight - 8));
    if (isActiveRegion) ImGui::PopStyleColor(2);

    if (ImGui::IsItemClicked(0) && ImGui::IsMouseDoubleClicked(0) && _tracks->GetActiveTrack() == track)
    {
        _state->ui._activeCenterScreen = 1;
    }
    else if (ImGui::IsItemClicked(0))
    {
        _mouseDragStart = ImGui::GetMousePos();
        _mouseDragTrack = track;
        _mouseDragFrom = region.first;

        _tracks->SetActiveTrack(track);
        _tracks->SetActiveRegion(track, region.first);
    }

    if (_mouseDragTrack == track)
    {
        if (_mouseDragFrom == region.first)
        {
            auto newX = GetNewRegionStart(region);

            if (ImGui::IsMouseReleased(0) || (!ImGui::IsMouseDown(0) && newX != region.first))
            {
                FinishDragRegion(newX);
            }
            else
            {
                auto overlayOrigin = ImVec2(trackScreenOrigin.x + StepsToPixels(newX), trackScreenOrigin.y + 4);

                auto min = ImVec2(
                    std::max(overlayOrigin.x, trackScreenOrigin.x + ImGui::GetScrollX()),
                    std::max(overlayOrigin.y, trackScreenOrigin.y + ImGui::GetScrollY()));

                auto max = ImVec2(
                    overlayOrigin.x + regionWidth,
                    overlayOrigin.y + finalTrackHeight - 8);

                ImGui::GetOverlayDrawList()->AddRectFilled(
                    min,
                    max,
                    ImColor(255, 255, 255, 50),
                    regionRounding);
            }
        }
    }

    RenderNotes(region, trackScreenOrigin, finalTrackHeight);

    ImGui::PopStyleVar(2);

    ImGui::PopID();
}

void TracksEditor::RenderNotes(
    std::pair<const long, Region> const &region,
    ImVec2 const &trackScreenOrigin,
    int finalTrackHeight)
{
    auto drawlist = ImGui::GetWindowDrawList();
    auto noteRange = region.second.GetMaxNote() - region.second.GetMinNote();
    auto regionScreenOrigin = ImVec2(trackScreenOrigin.x + StepsToPixels(region.first), trackScreenOrigin.y + 4);

    struct ActiveNote
    {
        std::chrono::milliseconds::rep time;
        uint32_t velocity;
    };

    std::map<uint32_t, ActiveNote> activeNotes;
    for (auto event : region.second.Events())
    {
        const int noteHeight = 5;
        auto h = (finalTrackHeight - ((regionRounding + noteHeight) * 3));

        if (event.first > region.second.Length())
        {
            break;
        }

        for (auto me : event.second)
        {
            auto found = activeNotes.find(me.num);
            auto y = h - (h * float(me.num - region.second.GetMinNote()) / float(noteRange));
            auto top = regionScreenOrigin.y + y + regionRounding + noteHeight;

            if (me.type == MidiEventTypes::M_NOTE)
            {
                drawlist->AddRectFilled(
                    ImVec2(regionScreenOrigin.x + StepsToPixels(event.first) - 1, top),
                    ImVec2(regionScreenOrigin.x + StepsToPixels(event.first) + 1, top + noteHeight),
                    ImColor(255, 0, 0, 255));

                if (me.value != 0 && found == activeNotes.end())
                {
                    activeNotes.insert(std::make_pair(me.num, ActiveNote{event.first, me.value}));
                    continue;
                }
                else if (me.value == 0 && found != activeNotes.end())
                {
                    auto start = StepsToPixels(found->second.time);
                    auto end = StepsToPixels(event.first);
                    drawlist->AddRectFilled(
                        ImVec2(regionScreenOrigin.x + start, top),
                        ImVec2(regionScreenOrigin.x + end, top + noteHeight),
                        ImColor(255, 255, 255, 255));

                    auto vel = (end - start) * (found->second.velocity / 128.0f);

                    drawlist->AddLine(
                        ImVec2(regionScreenOrigin.x + start, top + 2),
                        ImVec2(regionScreenOrigin.x + start + vel, top + 2),
                        ImColor(155, 155, 155, 255));

                    activeNotes.erase(found);
                }
            }

            else if (me.type == MidiEventTypes::M_PRESSURE)
            {
                drawlist->AddRectFilled(
                    ImVec2(regionScreenOrigin.x + StepsToPixels(event.first) - 1, top),
                    ImVec2(regionScreenOrigin.x + StepsToPixels(event.first) + 1, top + noteHeight),
                    ImColor(255, 0, 255, 255));
            }
            else
            {
                drawlist->AddRectFilled(
                    ImVec2(regionScreenOrigin.x + StepsToPixels(event.first) - 1, top),
                    ImVec2(regionScreenOrigin.x + StepsToPixels(event.first) + 1, top + noteHeight),
                    ImColor(255, 255, 0, 255));
            }
        }
    }
}

void TracksEditor::RenderTrack(
    ITrack *track,
    int t,
    int trackWidth)
{
    auto finalTrackHeight = _trackHeight;

    auto drawList = ImGui::GetWindowDrawList();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(track->GetColor()[0], track->GetColor()[1], track->GetColor()[2], track->GetColor()[3] * 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(track->GetColor()[0], track->GetColor()[1], track->GetColor()[2], track->GetColor()[3] * 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(track->GetColor()[0], track->GetColor()[1], track->GetColor()[2], track->GetColor()[3]));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(track->GetColor()[0] * 1.2f, track->GetColor()[1] * 1.2f, track->GetColor()[2] * 1.2f, track->GetColor()[3]));

    auto pp = ImGui::GetCursorScreenPos();
    auto ppNotScrolled = ImVec2(pp.x + ImGui::GetScrollX(), pp.y);
    if (track == _tracks->GetActiveTrack())
    {
        drawList->AddRectFilled(
            ppNotScrolled,
            ImVec2(ppNotScrolled.x + ImGui::GetContentRegionAvailWidth(), ppNotScrolled.y + finalTrackHeight),
            _trackactivebgcol);
    }
    else if (t % 2 == 0)
    {
        drawList->AddRectFilled(
            ppNotScrolled,
            ImVec2(ppNotScrolled.x + ImGui::GetContentRegionAvailWidth(), ppNotScrolled.y + finalTrackHeight),
            _trackbgcol);
    }
    else
    {
        drawList->AddRectFilled(
            ppNotScrolled,
            ImVec2(ppNotScrolled.x + ImGui::GetContentRegionAvailWidth(), ppNotScrolled.y + finalTrackHeight),
            _trackaltbgcol);
    }

    ImGui::PushID(t);

    auto trackOrigin = ImGui::GetCursorPos();
    auto trackScreenOrigin = ImGui::GetCursorScreenPos();

    for (auto &region : track->Regions())
    {
        RenderRegion(track, region, trackOrigin, trackScreenOrigin, finalTrackHeight);
    }

    if (_doMove && _mouseDragTrack == track)
    {
        auto r = track->GetRegion(_mouseDragFrom);
        if (!ImGui::GetIO().KeyShift)
        {
            track->RemoveRegion(_mouseDragFrom);
        }
        track->AddRegion(moveTo, r);

        _tracks->SetActiveRegion(track, moveTo);
        _mouseDragFrom = moveTo = -1;
        _doMove = false;
    }

    ImGui::SetCursorPos(trackOrigin);
    auto btnSize = ImVec2(std::max(trackWidth, int(ImGui::GetContentRegionAvailWidth())), finalTrackHeight);
    if (ImGui::InvisibleButton(track->GetName().c_str(), btnSize))
    {
        _tracks->SetActiveTrack(track);

        auto regionStart = PixelsToSteps(ImGui::GetMousePos().x - pp.x);
        regionStart = track->StartNewRegion(regionStart);
        if (regionStart >= 0)
        {
            _tracks->SetActiveRegion(track, regionStart);
        }
    }

    ImGui::PopID();
    ImGui::PopStyleColor(4);
}

void TracksEditor::RenderTrackHeader(
    ITrack *track,
    int t)
{
    if (track == nullptr)
    {
        return;
    }

    auto finalTrackHeight = _trackHeight;

    auto drawList = ImGui::GetWindowDrawList();
    auto cursorScreenPos = ImGui::GetCursorScreenPos();
    if (track == _tracks->GetActiveTrack())
    {
        drawList->AddRectFilled(cursorScreenPos, ImVec2(cursorScreenPos.x + trackHeaderWidth, cursorScreenPos.y + finalTrackHeight), _trackactivebgcol);
    }
    else if (t % 2 == 0)
    {
        drawList->AddRectFilled(cursorScreenPos, ImVec2(cursorScreenPos.x + trackHeaderWidth, cursorScreenPos.y + finalTrackHeight), _trackbgcol);
    }
    else
    {
        drawList->AddRectFilled(cursorScreenPos, ImVec2(cursorScreenPos.x + trackHeaderWidth, cursorScreenPos.y + finalTrackHeight), _trackaltbgcol);
    }

    ImGui::PushID(t);

    auto cursorPos = ImGui::GetCursorPos();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[(_tracks->GetActiveTrack() == track ? ImGuiCol_ButtonActive : ImGuiCol_Button)]);
    {
        std::stringstream ss;
        ss << (t + 1);
        if (ImGui::Button(ss.str().c_str(), ImVec2(20, finalTrackHeight)))
        {
            _tracks->SetActiveTrack(track);
        }
    }
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    ImGui::SameLine();

    ImGui::BeginGroup();
    {
        if (ImGui::Button(ICON_FAD_POWERSWITCH))
        {
            _tracks->RemoveTrack(track);
        }
        else
        {
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Remove this track");
            }

            ImGui::SameLine();

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, ImGui::GetStyle().ItemSpacing.y));
            {
                if (ImGui::ActiveButton(ICON_FAD_MUTE, track->IsMuted()))
                {
                    track->ToggleMuted();
                    if (track->IsMuted() && _tracks->GetSoloTrack() == track)
                    {
                        _tracks->SetSoloTrack(nullptr);
                    }
                    _tracks->SetActiveTrack(track);
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip(track->IsMuted() ? "Unmute this Track" : "Mute this Track");
                }

                ImGui::SameLine();

                if (ImGui::ActiveButton(ICON_FAD_SOLO, _tracks->GetSoloTrack() == track))
                {
                    if (_tracks->GetSoloTrack() != track)
                    {
                        _tracks->SetSoloTrack(track);
                        track->Unmute();
                    }
                    else
                    {
                        _tracks->SetSoloTrack(nullptr);
                    }
                    _tracks->SetActiveTrack(track);
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip(_tracks->GetSoloTrack() == track ? "Unsolo this Track" : "Solo this Track");
                }

                ImGui::SameLine();

                if (track->IsReadyForRecoding())
                {
                    if (_state->_recording)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 0, 0, 1));
                    }
                    else
                    {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0, 0, 1));
                    }
                }
                else
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_Button]);
                }

                if (ImGui::Button(ICON_FAD_RECORD))
                {
                    track->ToggleReadyForRecording();
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip(track->IsReadyForRecoding() ? "Stop being ready to record in this track" : "Get ready to record in this track");
                }

                ImGui::PopStyleColor();
            }
            ImGui::PopStyleVar();

            if (finalTrackHeight < 60)
            {
                ImGui::SameLine();
            }

            if (_editTrackName == t)
            {
                ImGui::SetKeyboardFocusHere();
                if (ImGui::InputText("##editName", _editTrackBuffer, 128, ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    track->SetName(_editTrackBuffer);
                    _editTrackName = -1;
                }
            }
            else
            {
                ImGui::Text("%s", track->GetName().c_str());
                if (ImGui::IsItemClicked())
                {
                    _editTrackName = t;
                    strcpy_s(_editTrackBuffer, 128, track->GetName().c_str());
                    _tracks->SetActiveTrack(track);
                }
            }
        }
    }
    ImGui::EndGroup();

    if (ImGui::BeginPopupContextItem("track context menu"))
    {
        if (ImGui::MenuItem("Remove track"))
        {
            _tracks->RemoveTrack(track);
        }

        ImGui::EndPopup();
    }

    ImGui::SetCursorPos(ImVec2(cursorPos.x, cursorPos.y + finalTrackHeight + ImGui::GetStyle().ItemSpacing.y));
    ImGui::PopID();
}
