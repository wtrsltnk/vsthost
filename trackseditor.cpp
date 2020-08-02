#include "trackseditor.h"

#include "IconsFontaudio.h"
#include "IconsForkAwesome.h"
#include "imguiutils.h"
#include <iostream>
#include <sstream>

const int trackHeaderWidth = 200;
const int trackToolsHeight = 30;
const int timelineHeight = 30;
const int regionRounding = 5.0f;
const int regionResizeHandleWidth = 20.0f;

const ImColor color = ImColor(55, 55, 55, 55);
const ImColor accentColor = ImColor(55, 55, 55, 155);
const ImColor timelineTextColor = ImColor(155, 155, 155, 255);

TracksEditor::TracksEditor() = default;

long TracksEditor::PixelsToSteps(
    float pixels)
{
    return (pixels / _pixelsPerStep) * 1000.0f;
}

float TracksEditor::StepsToPixels(
    long steps)
{
    return (steps / 1000.0f) * _pixelsPerStep;
}

int TracksEditor::MaxTracksWidth()
{
    for (auto &track : _tracks->tracks)
    {
        for (auto &region : track->_regions)
        {
            auto end = region.first + region.second._length + 8000;
            if (end > _maxTrackLength) _maxTrackLength = end;
        }
    }

    return _maxTrackLength;
}

int TracksEditor::EditTrackName() const
{
    return _editTrackName;
}

void TracksEditor::SetState(State *state)
{
    _state = state;
}
void TracksEditor::SetTracksManager(TracksManager *tracks)
{
    _tracks = tracks;
}

void TracksEditor::Render(const ImVec2 &pos, const ImVec2 &size)
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
        int fullHeight = _tracks->tracks.size() * (_trackHeight + ImGui::GetStyle().ItemSpacing.y);

        ImGui::BeginChild(
            "track_tools",
            ImVec2(0, trackToolsHeight));
        {
            if (ImGui::Button(ICON_FK_PLUS))
            {
                _tracks->activeTrack = _tracks->addVstTrack();
            }

            ImGui::SameLine();

            ImGui::PushItemWidth(100);
            ImGui::SliderInt("track height", &_trackHeight, 24, 300);
            ImGui::SameLine();
            ImGui::SliderInt("zoom", &(_pixelsPerStep), 8, 200);
            ImGui::PopItemWidth();
            ImGui::SameLine();

            static int e = _snapToPixels == 4000 ? 0 : 1;
            if (ImGui::RadioButton("Snap to bar", &e, 0))
            {
                _snapToPixels = 4000;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Snap to step", &e, 1))
            {
                _snapToPixels = 1000;
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
            "tracksContainer",
            ImVec2(size.x - trackHeaderWidth - ImGui::GetStyle().ItemSpacing.x, 0),
            false,
            ImGuiWindowFlags_HorizontalScrollbar);
        {
            const ImVec2 p = ImGui::GetCursorScreenPos();

            RenderGrid(p, trackWidth);

            ImGui::BeginGroup();
            {
                int trackIndex = 0;
                for (auto track : _tracks->tracks)
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

        RenderTimeline(
            ImVec2(trackScreenOrigin.x, trackScreenOrigin.y),
            trackWidth,
            _tracksScrollx);

        RenderCursor(trackScreenOrigin, size);

        ImGui::SetCursorPosY(contentTop + timelineHeight);
        ImGui::BeginChild(
            "headersContainer",
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
                for (auto track : _tracks->tracks)
                {
                    RenderTrackHeader(
                        track,
                        trackIndex++);
                }
            }
            ImGui::EndChild();
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

void TracksEditor::RenderCursor(
    ImVec2 const &p,
    ImVec2 const &size)
{
    auto cursor = ImVec2(p.x + StepsToPixels(_state->_cursor), p.y);
    ImGui::GetWindowDrawList()->AddLine(
        cursor,
        ImVec2(cursor.x, cursor.y + size.y - trackToolsHeight),
        ImGui::GetColorU32(ImGui::GetStyle().Colors[ImGuiCol_PlotHistogramHovered]),
        3.0f);
}

void TracksEditor::RenderTimeline(
    ImVec2 const &p,
    int windowWidth,
    int scrollX)
{
    int step = 1;
    for (int g = 0; g < windowWidth; g += (_pixelsPerStep * 4))
    {
        std::stringstream ss;
        ss << (step++);
        auto cursor = ImVec2(p.x - scrollX + g, p.y);
        if (cursor.x >= p.x)
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
}

void TracksEditor::RenderGrid(
    ImVec2 const &p,
    int windowWidth)
{
    int step = 0;
    int gg = 0;
    while (gg < std::max(windowWidth, int(ImGui::GetContentRegionAvailWidth())))
    {
        auto cursor = ImVec2(p.x + StepsToPixels(step), p.y);

        ImGui::GetWindowDrawList()->AddLine(
            cursor,
            ImVec2(cursor.x, cursor.y + (_trackHeight + ImGui::GetStyle().ItemSpacing.y) * _tracks->tracks.size() - ImGui::GetStyle().ItemSpacing.y),
            gg % (_pixelsPerStep * 4) == 0 ? accentColor : color,
            1.0f);

        step += 1000;
        gg += _pixelsPerStep;
    }
}

void TracksEditor::FinishDragRegion(
    long newX)
{
    doMove = true;
    moveTo = newX;
    _mouseDragStart = ImGui::GetMousePos();
}

long TracksEditor::GetNewRegionStart(
    std::pair<long, Region> region)
{
    auto diff = ImVec2(ImGui::GetMousePos().x - _mouseDragStart.x, ImGui::GetMousePos().y - _mouseDragStart.y);
    auto newStart = region.first + PixelsToSteps(diff.x) - (int(PixelsToSteps(diff.x)) % _snapToPixels);

    newStart = newStart - (int(newStart) % _snapToPixels);

    if (newStart < 0) newStart = 0;

    return newStart;
}

long TracksEditor::GetNewRegionLength(
    std::pair<long, Region> region)
{
    auto diff = ImVec2(ImGui::GetMousePos().x - _mouseDragStart.x, ImGui::GetMousePos().y - _mouseDragStart.y);
    auto snappedDiffX = PixelsToSteps(diff.x) - (int(PixelsToSteps(diff.x)) % _snapToPixels);
    auto newLength = region.second._length + snappedDiffX;

    newLength = newLength - (int(newLength) % _snapToPixels);

    return newLength;
}

void TracksEditor::StartDragRegion(
    Track *track,
    std::pair<long, Region> region)
{
    _tracks->activeTrack = track;
    _mouseDragStart = ImGui::GetMousePos();
    _tracks->activeRegion = std::tuple<Track *, long>(track, region.first);
}

void TracksEditor::RenderRegion(
    Track *track,
    std::pair<const long, Region> &region,
    ImVec2 const &trackOrigin,
    ImVec2 const &trackScreenOrigin)
{
    auto drawlist = ImGui::GetWindowDrawList();

    auto noteRange = region.second.GetMaxNote() - region.second.GetMinNote();

    auto regionOrigin = ImVec2(trackOrigin.x + StepsToPixels(region.first), trackOrigin.y + 4);
    auto regionScreenOrigin = ImVec2(trackScreenOrigin.x + StepsToPixels(region.first), trackScreenOrigin.y + 4);
    auto regionWidth = StepsToPixels(region.second._length);

    ImGui::PushID(region.first);

    auto resizeHandlePosition = ImVec2(regionOrigin.x + regionWidth - regionResizeHandleWidth, regionOrigin.y);
    ImGui::SetCursorPos(resizeHandlePosition);

    ImGui::Button(ICON_FK_ARROWS_H, ImVec2(regionResizeHandleWidth, _trackHeight - 8));

    if (ImGui::IsItemClicked(0))
    {
        StartDragRegion(track, region);
    }

    if (ImGui::IsItemActive())
    {
        auto newLength = GetNewRegionLength(region);

        if (newLength > 0 && newLength != region.second._length)
        {
            region.second._length = newLength;
            _mouseDragStart = ImGui::GetMousePos();
        }
    }

    ImGui::SetCursorPos(regionOrigin);

    auto isActiveRegion = _tracks->activeTrack == track && std::get<long>(_tracks->activeRegion) == region.first;

    if (isActiveRegion)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(track->_color[0] * 2.0f, track->_color[1] * 2.0f, track->_color[2] * 2.0f, track->_color[3] * 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(track->_color[0] * 2.0f, track->_color[1] * 2.0f, track->_color[2] * 2.0f, track->_color[3] * 0.7f));
    }
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, regionRounding);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::Button("##test", ImVec2(regionWidth, _trackHeight - 8));
    if (isActiveRegion) ImGui::PopStyleColor(2);

    if (ImGui::IsItemClicked(0))
    {
        _mouseDragStart = ImGui::GetMousePos();
        _mouseDragTrack = track;
        _mouseDragFrom = region.first;

        _tracks->activeTrack = track;
        _tracks->activeRegion = std::tuple<Track *, long>(track, region.first);
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
                    overlayOrigin.y + _trackHeight - 8);

                ImGui::GetOverlayDrawList()->AddRectFilled(
                    min,
                    max,
                    ImColor(255, 255, 255, 100));
            }
        }
    }

    ImGui::PopStyleVar(2);

    struct ActiveNote
    {
        std::chrono::milliseconds::rep time;
        unsigned int velocity;
    };

    std::map<unsigned int, ActiveNote> activeNotes;
    for (auto event : region.second._events)
    {
        const int noteHeight = 5;
        auto h = (_trackHeight - ((regionRounding + noteHeight) * 3));

        if (event.first > region.second._length)
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
    ImGui::PopID();
}

void TracksEditor::RenderTrack(
    Track *track,
    int t,
    int trackWidth)
{
    auto drawList = ImGui::GetWindowDrawList();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(track->_color[0], track->_color[1], track->_color[2], track->_color[3] * 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(track->_color[0], track->_color[1], track->_color[2], track->_color[3] * 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(track->_color[0], track->_color[1], track->_color[2], track->_color[3]));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(track->_color[0] * 1.2f, track->_color[1] * 1.2f, track->_color[2] * 1.2f, track->_color[3]));

    auto pp = ImGui::GetCursorScreenPos();
    if (track == _tracks->activeTrack)
    {
        drawList->AddRectFilled(pp, ImVec2(pp.x + ImGui::GetContentRegionAvailWidth(), pp.y + _trackHeight), _trackactivebgcol);
    }
    else if (t % 2 == 0)
    {
        drawList->AddRectFilled(pp, ImVec2(pp.x + ImGui::GetContentRegionAvailWidth(), pp.y + _trackHeight), _trackbgcol);
    }
    else
    {
        drawList->AddRectFilled(pp, ImVec2(pp.x + ImGui::GetContentRegionAvailWidth(), pp.y + _trackHeight), _trackaltbgcol);
    }

    ImGui::PushID(t);

    auto trackOrigin = ImGui::GetCursorPos();
    auto trackScreenOrigin = ImGui::GetCursorScreenPos();

    for (auto &region : track->_regions)
    {
        RenderRegion(track, region, trackOrigin, trackScreenOrigin);
    }

    if (doMove && _mouseDragTrack == track)
    {
        auto r = track->_regions[_mouseDragFrom];
        if (!ImGui::GetIO().KeyShift)
        {
            track->_regions.erase(_mouseDragFrom);
        }
        track->_regions.insert(std::make_pair(moveTo, r));
        _tracks->activeRegion = std::tuple<Track *, long>(track, moveTo);
        _mouseDragFrom = moveTo = -1;
        doMove = false;
    }

    ImGui::SetCursorPos(trackOrigin);
    auto btnSize = ImVec2(std::max(trackWidth, int(ImGui::GetContentRegionAvailWidth())), _trackHeight);
    if (ImGui::InvisibleButton(track->_name.c_str(), btnSize))
    {
        _tracks->activeTrack = track;

        auto regionStart = PixelsToSteps(ImGui::GetMousePos().x - pp.x);
        regionStart = track->StartNewRegion(regionStart);
        if (regionStart >= 0)
        {
            _tracks->activeRegion = std::tuple<Track *, long>(track, regionStart);
        }
    }

    ImGui::PopID();
    ImGui::PopStyleColor(4);
}

void TracksEditor::RenderTrackHeader(
    Track *track,
    int t)
{
    auto drawList = ImGui::GetWindowDrawList();
    auto cursorScreenPos = ImGui::GetCursorScreenPos();
    if (track == _tracks->activeTrack)
    {
        drawList->AddRectFilled(cursorScreenPos, ImVec2(cursorScreenPos.x + trackHeaderWidth, cursorScreenPos.y + _trackHeight), _trackactivebgcol);
    }
    else if (t % 2 == 0)
    {
        drawList->AddRectFilled(cursorScreenPos, ImVec2(cursorScreenPos.x + trackHeaderWidth, cursorScreenPos.y + _trackHeight), _trackbgcol);
    }
    else
    {
        drawList->AddRectFilled(cursorScreenPos, ImVec2(cursorScreenPos.x + trackHeaderWidth, cursorScreenPos.y + _trackHeight), _trackaltbgcol);
    }

    ImGui::PushID(t);

    auto cursorPos = ImGui::GetCursorPos();

    std::stringstream ss;
    ss << (t + 1);
    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[(_tracks->activeTrack == track ? ImGuiCol_ButtonActive : ImGuiCol_Button)]);
    if (ImGui::Button(ss.str().c_str(), ImVec2(20, _trackHeight)))
    {
        _tracks->activeTrack = track;
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();

    ImGui::BeginGroup();
    {
        if (ImGui::Button(ICON_FAD_POWERSWITCH))
        {
            _tracks->removeTrack(track);
        }

        ImGui::SameLine();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, ImGui::GetStyle().ItemSpacing.y));
        {
            if (ImGui::ActiveButton(ICON_FAD_MUTE, track->_muted))
            {
                track->_muted = !track->_muted;
                if (track->_muted && _tracks->soloTrack == track)
                {
                    _tracks->soloTrack = nullptr;
                }
                _tracks->activeTrack = track;
            }

            ImGui::SameLine();

            if (ImGui::ActiveButton(ICON_FAD_SOLO, _tracks->soloTrack == track))
            {
                if (_tracks->soloTrack != track)
                {
                    _tracks->soloTrack = track;
                    track->_muted = false;
                }
                else
                {
                    _tracks->soloTrack = nullptr;
                }
                _tracks->activeTrack = track;
            }

            ImGui::SameLine();

            if (track->_readyForRecord)
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
                track->_readyForRecord = !track->_readyForRecord;
            }

            ImGui::PopStyleColor();
        }
        ImGui::PopStyleVar();

        if (_trackHeight < 60)
        {
            ImGui::SameLine();
        }

        if (_editTrackName == t)
        {
            ImGui::SetKeyboardFocusHere();
            if (ImGui::InputText("##editName", _editTrackBuffer, 128, ImGuiInputTextFlags_EnterReturnsTrue))
            {
                track->_name = _editTrackBuffer;
                _editTrackName = -1;
            }
        }
        else
        {
            ImGui::Text("%s", track->_name.c_str());
            if (ImGui::IsItemClicked())
            {
                _editTrackName = t;
                strcpy_s(_editTrackBuffer, 128, track->_name.c_str());
                _tracks->activeTrack = track;
            }
        }
    }
    ImGui::EndGroup();

    if (ImGui::BeginPopupContextItem("track context menu"))
    {
        if (ImGui::MenuItem("Remove track"))
        {
            _tracks->removeTrack(track);
        }

        ImGui::EndPopup();
    }

    ImGui::SetCursorPos(ImVec2(cursorPos.x, cursorPos.y + _trackHeight + ImGui::GetStyle().ItemSpacing.y));
    ImGui::PopID();
}
