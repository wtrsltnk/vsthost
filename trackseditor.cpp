#include "trackseditor.h"

#include "IconsFontaudio.h"
#include "IconsForkAwesome.h"
#include "imguiutils.h"
#include <iostream>
#include <sstream>

TracksEditor::TracksEditor()
{
}

//std::chrono::milliseconds::rep TracksEditor::PixelsToTime(
//    float pixels)
//{
//    auto stepsPerSecond = (_state->_bpm * 4.0) / 60.0;
//    auto msPerStep = 1000.0 / stepsPerSecond;

//    return (pixels / _pixelsPerStep) * msPerStep;
//}

//float TracksEditor::TimeToPixels(
//    std::chrono::milliseconds::rep time)
//{
//    auto stepsPerSecond = (_state->_bpm * 4.0) / 60.0;
//    auto msPerStep = 1000.0 / stepsPerSecond;

//    return (time / msPerStep) * _pixelsPerStep;
//}

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
    int max = 0;

    for (auto &track : _tracks->tracks)
    {
        for (auto &region : track->_regions)
        {
            auto end = region.first + region.second._length;
            if (end > max) max = end;
        }
    }

    if (max < 1000)
    {
        return 1000;
    }

    return max;
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

const int trackHeaderWidth = 200;
const int trackToolsHeight = 30;
const int timelineHeight = 30;

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

        auto trackWidth = MaxTracksWidth();
        int fullHeight = _tracks->tracks.size() * (_trackHeight + ImGui::GetStyle().ItemSpacing.y);

        ImGui::BeginChild(
            "track_tools",
            ImVec2(size.y, trackToolsHeight));
        {
            ImGui::Button(ICON_FK_PLUS);
            if (ImGui::BeginPopupContextItem("item context menu", 0))
            {
                if (ImGui::MenuItem("VST"))
                {
                    _tracks->activeTrack = _tracks->addVstTrack();
                }

                ImGui::EndPopup();
            }

            ImGui::SameLine();

            ImGui::PushItemWidth(100);
            ImGui::SliderInt("track height", &_trackHeight, 60, 300);
            ImGui::SameLine();
            ImGui::SliderInt("zoom", &(_pixelsPerStep), 8, 200);
            ImGui::PopItemWidth();
        }
        ImGui::EndChild();

        float _tracksScrolly = 0;

        auto contentTop = ImGui::GetCursorPosY();
        ImGui::MoveCursorPos(ImVec2(trackHeaderWidth, 0));
        ImGui::BeginChild(
            "tracksContainer",
            ImVec2(size.x - trackHeaderWidth - ImGui::GetStyle().ItemSpacing.x, -10),
            false,
            ImGuiWindowFlags_HorizontalScrollbar);
        {
            const ImVec2 p = ImGui::GetCursorScreenPos();

            RenderGrid(p, trackWidth);

            RenderTimeline(p, trackWidth);

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

            RenderCursor(p);

            _tracksScrolly = ImGui::GetScrollY();
        }
        ImGui::EndChild();

        ImGui::SetCursorPosY(contentTop + 30);
        ImGui::BeginChild(
            "headersContainer",
            ImVec2(trackHeaderWidth, -(10)),
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
                        trackIndex++,
                        trackWidth);
                }
            }
            ImGui::EndChild();
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

void TracksEditor::RenderCursor(
    ImVec2 const &p)
{
    auto cursor = ImVec2(p.x + StepsToPixels(_state->_cursor), p.y);
    ImGui::GetWindowDrawList()->AddLine(
        cursor,
        ImVec2(cursor.x, cursor.y + timelineHeight + (_trackHeight + ImGui::GetStyle().ItemSpacing.y) * _tracks->tracks.size() - ImGui::GetStyle().ItemSpacing.y),
        ImGui::GetColorU32(ImGui::GetStyle().Colors[ImGuiCol_PlotHistogramHovered]),
        3.0f);
}

const ImColor color = ImColor(55, 55, 55, 55);
const ImColor accentColor = ImColor(55, 55, 55, 155);
const ImColor timelineTextColor = ImColor(155, 155, 155, 255);

void TracksEditor::RenderTimeline(
    ImVec2 const &p,
    int trackWidth)
{
    int step = 1;
    for (int g = 0; g < trackWidth; g += (_pixelsPerStep * 4))
    {
        std::stringstream ss;
        ss << (step++);
        auto cursor = ImVec2(p.x + g + 5, p.y);
        ImGui::GetWindowDrawList()->AddText(cursor, timelineTextColor, ss.str().c_str());
    }

    ImGui::MoveCursorPos(ImVec2(0, timelineHeight));
}

void TracksEditor::RenderGrid(
    ImVec2 const &p,
    int trackWidth)
{
    int step = 0;
    int gg = 0;
    while (gg < trackWidth)
    {
        auto cursor = ImVec2(p.x + StepsToPixels(step), p.y);
        ImGui::GetWindowDrawList()->AddLine(
            cursor,
            ImVec2(cursor.x, cursor.y + timelineHeight + (_trackHeight + ImGui::GetStyle().ItemSpacing.y) * _tracks->tracks.size() - ImGui::GetStyle().ItemSpacing.y),
            gg % (_pixelsPerStep * 4) == 0 ? accentColor : color,
            1.0f);
        step += 1000;
        gg += _pixelsPerStep;
    }

    //    for (int g = 0; g < trackWidth; g += _pixelsPerStep)
    //    {
    //        auto cursor = ImVec2(p.x + g, p.y);
    //        ImGui::GetWindowDrawList()->AddLine(
    //            cursor,
    //            ImVec2(cursor.x, cursor.y + timelineHeight + (_trackHeight + ImGui::GetStyle().ItemSpacing.y) * _tracks->tracks.size() - ImGui::GetStyle().ItemSpacing.y),
    //            g % (_pixelsPerStep * 4) == 0 ? accentColor : color,
    //            1.0f);
    //    }
}

void TracksEditor::RenderTrack(
    Track *track,
    int t,
    int trackWidth)
{
    auto drawList = ImGui::GetWindowDrawList();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(track->_color[0], track->_color[1], track->_color[2], track->_color[3]));
    auto pp = ImGui::GetCursorScreenPos();
    if (track == _tracks->activeTrack)
    {
        drawList->AddRectFilled(pp, ImVec2(pp.x + trackWidth, pp.y + _trackHeight), _trackactivebgcol);
    }
    else if (t % 2 == 0)
    {
        drawList->AddRectFilled(pp, ImVec2(pp.x + trackWidth, pp.y + _trackHeight), _trackbgcol);
    }
    else
    {
        drawList->AddRectFilled(pp, ImVec2(pp.x + trackWidth, pp.y + _trackHeight), _trackaltbgcol);
    }

    ImGui::PushID(t);

    auto trackOrigin = ImGui::GetCursorPos();
    if (ImGui::InvisibleButton(track->_name.c_str(), ImVec2(trackWidth, _trackHeight)))
    {
        auto regionStart = PixelsToSteps(ImGui::GetMousePos().x - pp.x);

        regionStart = regionStart - (regionStart % 4000);

        Region region;
        region._length = 4000;

        track->_regions.insert(std::make_pair(regionStart, region));

        _tracks->activeTrack = track;
    }

    for (auto region : track->_regions)
    {
        auto noteRange = region.second.GetMaxNote() - region.second.GetMinNote();

        auto regionOrigin = ImVec2(trackOrigin.x + StepsToPixels(region.first), trackOrigin.y + 4);
        ImGui::PushID(region.first);
        ImGui::SetCursorPos(regionOrigin);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
        ImGui::Button("##test", ImVec2(StepsToPixels(region.second._length), _trackHeight - 8));
        ImGui::PopStyleVar();

        struct ActiveNote
        {
            std::chrono::milliseconds::rep time;
            unsigned int velocity;
        };

        std::map<unsigned int, ActiveNote> activeNotes;
        for (auto event : region.second._events)
        {
            const int noteHeight = 5;
            auto h = (_trackHeight - (noteHeight * 3));

            for (auto me : event.second)
            {
                auto found = activeNotes.find(me.num);
                auto y = h - (h * float(me.num - region.second.GetMinNote()) / float(noteRange));
                auto top = pp.y + y + noteHeight;

                if (me.type == MidiEventTypes::M_NOTE)
                {
                    ImGui::GetWindowDrawList()->AddRectFilled(
                        ImVec2(pp.x + StepsToPixels(region.first) + StepsToPixels(event.first) - 1, top),
                        ImVec2(pp.x + StepsToPixels(region.first) + StepsToPixels(event.first) + 1, top + noteHeight),
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
                        ImGui::GetWindowDrawList()->AddRectFilled(
                            ImVec2(pp.x + StepsToPixels(region.first) + start, top),
                            ImVec2(pp.x + StepsToPixels(region.first) + end, top + noteHeight),
                            ImColor(255, 255, 255, 255));

                        auto vel = (end - start) * (found->second.velocity / 128.0f);

                        ImGui::GetWindowDrawList()->AddLine(
                            ImVec2(pp.x + StepsToPixels(region.first) + start, top + 2),
                            ImVec2(pp.x + StepsToPixels(region.first) + start + vel, top + 2),
                            ImColor(155, 155, 155, 255));

                        activeNotes.erase(found);
                    }
                }

                else if (me.type == MidiEventTypes::M_PRESSURE)
                {
                    ImGui::GetWindowDrawList()->AddRectFilled(
                        ImVec2(pp.x + StepsToPixels(region.first) + StepsToPixels(event.first) - 1, top),
                        ImVec2(pp.x + StepsToPixels(region.first) + StepsToPixels(event.first) + 1, top + noteHeight),
                        ImColor(255, 0, 255, 255));
                }
                else
                {
                    ImGui::GetWindowDrawList()->AddRectFilled(
                        ImVec2(pp.x + StepsToPixels(region.first) + StepsToPixels(event.first) - 1, top),
                        ImVec2(pp.x + StepsToPixels(region.first) + StepsToPixels(event.first) + 1, top + noteHeight),
                        ImColor(255, 255, 0, 255));
                }
            }
        }
        ImGui::PopID();
    }

    ImGui::PopID();
    ImGui::PopStyleColor();
}

void TracksEditor::RenderTrackHeader(
    Track *track,
    int t,
    int trackWidth)
{
    auto drawList = ImGui::GetWindowDrawList();
    auto pp = ImGui::GetCursorScreenPos();
    if (track == _tracks->activeTrack)
    {
        drawList->AddRectFilled(pp, ImVec2(pp.x + trackWidth, pp.y + _trackHeight), _trackactivebgcol);
    }
    else if (t % 2 == 0)
    {
        drawList->AddRectFilled(pp, ImVec2(pp.x + trackWidth, pp.y + _trackHeight), _trackbgcol);
    }
    else
    {
        drawList->AddRectFilled(pp, ImVec2(pp.x + trackWidth, pp.y + _trackHeight), _trackaltbgcol);
    }

    ImGui::PushID(t);

    auto p = ImGui::GetCursorPos();

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

    ImGui::SetCursorPos(ImVec2(p.x, p.y + _trackHeight + ImGui::GetStyle().ItemSpacing.y));
    ImGui::PopID();
}
