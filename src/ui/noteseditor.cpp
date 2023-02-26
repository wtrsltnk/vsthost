#include "noteseditor.h"

#include "../arpeggiatorpreviewservice.h"
#include "../imguiutils.h"
#include "../notepreviewservice.h"
#include "pianowindow.h"
#include <IconsFontaudio.h>
#include <IconsForkAwesome.h>
#include <midinote.h>
#include <region.h>
#include <track.h>

#include <algorithm>
#include <imgui_internal.h>
#include <iostream>
#include <spdlog/spdlog.h>

const int midiEventHeight = 15;
const int minPixelsPerStep = 8;
const int maxPixelsPerStep = 200;

NotesEditor::NotesEditor() = default;

void NotesEditor::Init()
{
    _monofont = ImGui::GetIO().Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\lucon.ttf", 10.0f);
}

void NotesEditor::Render(
    const ImVec2 &pos,
    const ImVec2 &size)
{
    ImGui::Begin(
        "NotesEditor",
        nullptr,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
    {
        ImGui::SetWindowPos(pos);
        ImGui::SetWindowSize(size);

        ImGui::BeginChild(
            "noteseditor_tools",
            ImVec2(0.0f, float(trackToolsHeight)));
        {
            if (ImGui::Button(ICON_FK_CHEVRON_LEFT))
            {
                _state->ui._activeCenterScreen = 0;
            }

            ImGui::SameLine();

            ImGui::PushItemWidth(100);
            ImGui::Text("zoom H :");
            ImGui::SameLine();
            ImGui::SliderInt("##zoom", &(_pixelsPerStep), minPixelsPerStep, maxPixelsPerStep);
            ImGui::PopItemWidth();
        }
        ImGui::EndChild();

        RenderTrackNotes(size);
    }
    ImGui::End();

    HandleNotesEditorShortCuts();
}

void NotesEditor::RenderTrackNotes(
    const ImVec2 &size)
{
    auto trackId = std::get<uint32_t>(_state->_tracks->GetActiveRegion());
    auto regionStart = std::get<std::chrono::milliseconds::rep>(_state->_tracks->GetActiveRegion());

    if (trackId == Track::Null || regionStart < 0)
    {
        ImGui::Text("No region selected");

        return;
    }

    float _tracksScrollx = 0;
    const float offset = 50.0f;

    auto &region = _state->_tracks
                       ->GetTrack(trackId)
                       .GetRegion(regionStart);

    auto timelineScreenPos = ImGui::GetCursorScreenPos();

    ImGui::MoveCursorPos(ImVec2(0.0f, float(timelineHeight)));

    auto noteNamesLocalPos = ImGui::GetCursorPos();

    ImGui::MoveCursorPos(ImVec2(offset, 0.0f));

    ImGui::BeginChild(
        "NotesContainer",
        ImVec2(0, 0),
        false,
        ImGuiWindowFlags_HorizontalScrollbar);
    {
        auto origin = ImGui::GetCursorPos();

        RenderGrid(ImGui::GetCursorScreenPos(), region.Length(), 127 * midiEventHeight);

        PushPopStyleVar(
            ImGuiStyleVar_ItemSpacing, ImVec2(0, 1),
            [&]() {
                ImGui::BeginGroup();
                {
                    RenderEventButtonsInRegion(region, ImVec2(timelineScreenPos.x + offset, timelineScreenPos.y), origin);

                    ImGui::SetCursorPos(origin);

                    RenderNoteHelpersInRegion(region, ImVec2(timelineScreenPos.x + offset, timelineScreenPos.y), origin);
                }
                ImGui::EndGroup();
            });

        ImGui::SetCursorPos(origin);

        RenderNotesCanvas(region);

        _tracksScrollx = ImGui::GetScrollX();
    }

    auto scrollAmount = ImGui::GetScrollY();
    ImGui::EndChild();

    RenderTimeline(
        ImVec2(timelineScreenPos.x + offset, timelineScreenPos.y),
        int(StepsToPixels(region.Length())),
        int(_tracksScrollx));

    RenderCursor(
        ImVec2(timelineScreenPos.x + offset - StepsToPixels(regionStart), timelineScreenPos.y),
        ImVec2(StepsToPixels(region.Length()), midiEventHeight * (127 - 21)),
        int(_tracksScrollx),
        0);

    ImGui::SetCursorPos(noteNamesLocalPos);

    ImGui::BeginChild(
        "NoteNamesContainer",
        ImVec2(offset, 0),
        false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    {
        ImGui::SetScrollY(scrollAmount);

        // Render the note names in front of a note "line" in the editor
        ImGui::PushFont(_monofont);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

        for (int noteNumber = 127; noteNumber >= 21; noteNumber--)
        {
            ImGui::PushID(noteNumber);

            ImGui::PushStyleColor(ImGuiCol_Button, noteNumber % 2 ? ImVec4(0, 0, 0, 0.1f) : ImVec4(0, 0, 0, 0));

            auto noteName = NoteToString(noteNumber);
            auto originNoteScreenPos = ImGui::GetCursorScreenPos();

            if (ImGui::Button(noteName, ImVec2(offset, midiEventHeight)))
            {
                spdlog::debug("{} - {}", noteName, noteNumber);
                _notePreviewService.PreviewNote(uint32_t(noteNumber), 100, 100);
            }

            ImGui::PopStyleColor();
            ImGui::PopID();
        }

        ImGui::PopStyleVar(2);
        ImGui::PopFont();
    }
    ImGui::EndChild();
}

void NotesEditor::RenderEventButtonsInRegion(
    Region &region,
    const ImVec2 &originContainerScreenPos,
    const ImVec2 &origin)
{
    long eindex = 0;
    static ImGuiID movingEventId;

    // Render the events with a square button and an orange dot
    for (const auto &t : region.Events())
    {
        for (const auto &midiEvent : t.second)
        {
            ImGui::PushID(eindex++);

            auto x = StepsToPixels(t.first);
            auto y = (127 - midiEvent.num) * midiEventHeight + 1;
            auto startPos = ImVec2(origin.x + x - (midiEventHeight / 2), origin.y + y);

            ImGui::SetCursorPos(startPos);

            if (ImGui::ButtonEx("event", ImVec2(midiEventHeight, midiEventHeight - 1), ImGuiButtonFlags_PressedOnClick))
            {
                movingEventId = ImGui::GetID("event");
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDown(1))
            {
                region.RemoveEvent(t.first, midiEvent.num);
            }

            if (ImGui::IsItemHovered())
            {
                if (midiEvent.type == MidiEventTypes::M_NOTE && midiEvent.value > 0)
                {
                    ImGui::SetTooltip("Move Note Down event");
                }
                if (midiEvent.type == MidiEventTypes::M_NOTE && midiEvent.value <= 0)
                {
                    ImGui::SetTooltip("Move Note Release event");
                }
            }

            auto center = ImVec2(
                originContainerScreenPos.x + x,
                originContainerScreenPos.y + y + timelineHeight);

            if (ImGui::IsItemActive())
            {
                ImGui::GetWindowDrawList()->AddLine(
                    ImVec2(
                        ImGui::GetIO().MouseClickedPos[0].x - ImGui::GetScrollX(),
                        center.y + (midiEventHeight * 0.5f) - ImGui::GetScrollY()),
                    ImVec2(
                        ImGui::GetIO().MousePos.x - ImGui::GetScrollX(),
                        center.y + (midiEventHeight * 0.5f) - ImGui::GetScrollY()),
                    ImGui::GetColorU32(ImGuiCol_PlotLines),
                    2.0f);
            }

            if (movingEventId == ImGui::GetID("event") && ImGui::IsMouseReleased(0))
            {
                auto move = SnapNotesSteps(t.first + PixelsToSteps(ImGui::GetIO().MousePos.x - ImGui::GetIO().MouseClickedPos[0].x));

                if (move < 0)
                {
                    move = 0;
                }

                region.MoveEvent(
                    midiEvent,
                    t.first,
                    move);

                movingEventId = 0;
            }

            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(
                    center.x - (midiEventHeight * 0.125f) - ImGui::GetScrollX(),
                    center.y + midiEventHeight * 0.375f - ImGui::GetScrollY()),
                ImVec2(
                    center.x + (midiEventHeight * 0.125f) - ImGui::GetScrollX(),
                    center.y + midiEventHeight * 0.625f - ImGui::GetScrollY()),
                ImColor(ImGui::GetStyle().Colors[ImGuiCol_PlotLinesHovered]),
                midiEventHeight / 3.0f);

            ImGui::PopID();
        }
    }
}

void NotesEditor::RenderNoteHelpersInRegion(
    Region &region,
    const ImVec2 &originContainerScreenPos,
    const ImVec2 &origin)
{
    auto notes = MidiNote::ConvertMidiEventsToMidiNotes(region.Events());

    // Render the time between events as a whole note
    for (int noteNumber = 127; noteNumber >= 21; noteNumber--)
    {
        auto originNotePos = ImGui::GetCursorPos();
        auto originNoteScreenPos = ImGui::GetCursorScreenPos();

        auto gridWidth = std::max(StepsToPixels(region.Length()), ImGui::GetWindowWidth());
        auto noteInOctave = noteNumber % 12;
        if (noteInOctave == Note_C_OffsetFromC || noteInOctave == Note_D_OffsetFromC || noteInOctave == Note_E_OffsetFromC || noteInOctave == Note_F_OffsetFromC || noteInOctave == Note_G_OffsetFromC || noteInOctave == Note_A_OffsetFromC || noteInOctave == Note_B_OffsetFromC)
        {
            ImGui::GetWindowDrawList()->AddRectFilled(
                ImVec2(
                    originNoteScreenPos.x,
                    originNoteScreenPos.y + 1),
                ImVec2(
                    originNoteScreenPos.x + gridWidth,
                    originNoteScreenPos.y + midiEventHeight),
                ImColor(1.0f, 1.0f, 1.0f, 0.1f));
        }

        ImGui::GetWindowDrawList()->AddLine(
            originNoteScreenPos,
            ImVec2(originNoteScreenPos.x + gridWidth, originNoteScreenPos.y),
            ImColor(0.7f, 0.7f, 0.7f, 0.2f));

        ImGui::PushID(noteNumber);
        for (const auto &notesInTime : notes[noteNumber])
        {
            for (const auto &note : notesInTime.second)
            {
                PushPopID(
                    int(note.length),
                    [&]() {
                        ImGui::SetCursorPos(
                            ImVec2(StepsToPixels(notesInTime.first) + (midiEventHeight * 0.5f), originNotePos.y + 1));

                        RenderEditableNote(
                            region,
                            noteNumber,
                            notesInTime.first,
                            note.length,
                            note.velocity,
                            ImVec2(StepsToPixels(note.length) - midiEventHeight + 1, midiEventHeight - 1),
                            originContainerScreenPos);
                    });
            }
        }
        ImGui::PopID();

        ImGui::SetCursorPos(ImVec2(originNotePos.x, originNotePos.y + midiEventHeight));
    }
}

void NotesEditor::RenderEditableNote(
    Region &region,
    int noteNumber,
    std::chrono::milliseconds::rep start,
    std::chrono::milliseconds::rep length,
    uint32_t velocity,
    const ImVec2 &noteSize,
    const ImVec2 &origin)
{
    static ImGuiID movingNoteId;

    if (noteSize.x <= 0)
    {
        return;
    }

    auto buttonPos = ImGui::GetCursorScreenPos();
    PushPopStyleColor(
        ImGuiCol_ButtonActive, ImGui::GetStyle().Colors[ImGuiCol_Button],
        [&]() {
            if (ImGui::ButtonEx("##note", noteSize, ImGuiButtonFlags_PressedOnClick))
            {
                movingNoteId = ImGui::GetID("##note");
                if (ImGui::GetIO().KeyShift)
                {
                    // add/remove note to/from selection
                }
                else
                {
                    // reset selection and make only this note the only selected note
                }
            }
        });

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Move whole Note");
    }

    auto diffy = ImGui::GetMousePos().y - _noteDrawingAndEditingStart.y;
    diffy -= (int(std::floor(diffy)) % midiEventHeight);

    if (ImGui::IsItemActive() && _editingNotes)
    {
        auto moveSteps = PixelsToSteps(ImGui::GetMousePos().x - _noteDrawingAndEditingStart.x);

        auto newx = SnapNotesSteps(start + moveSteps);

        if (newx < 0)
        {
            newx = 0;
        }

        auto noteStart = ImVec2(
            origin.x + StepsToPixels(newx),
            buttonPos.y + diffy);

        ImGui::GetWindowDrawList()->AddRectFilled(
            noteStart,
            ImVec2(noteStart.x + noteSize.x, noteStart.y + noteSize.y),
            ImColor(ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram]));

        if (_lastNotePreviewY != diffy)
        {
            auto amountToShiftNote = std::floor(diffy / float(midiEventHeight));

            _notePreviewService.PreviewNote(uint32_t(noteNumber - amountToShiftNote), 100, 100);

            _lastNotePreviewY = diffy;
        }
    }

    if (ImGui::IsItemHovered() && ImGui::IsMouseDown(0) && !_editingNotes)
    {
        _editingNotes = true;
        _noteDrawingAndEditingStart = ImGui::GetMousePos();
    }
    if (ImGui::IsItemHovered() && ImGui::IsMouseDown(1) && !_editingNotes)
    {
        region.RemoveEvent(start, noteNumber);
        region.RemoveEvent(start + length, noteNumber);
    }
    else if (ImGui::GetID("##note") == movingNoteId && _editingNotes && ImGui::IsMouseReleased(0))
    {
        _editingNotes = false;
        auto amountToShiftNote = std::floor(diffy / float(midiEventHeight));

        auto from = SnapNotesSteps(start + PixelsToSteps(ImGui::GetMousePos().x - _noteDrawingAndEditingStart.x));

        if (from < 0)
        {
            from = 0;
        }

        region.AddEvent(
            from,
            static_cast<uint32_t>(noteNumber - amountToShiftNote),
            true,
            velocity);

        region.AddEvent(
            from + length,
            static_cast<uint32_t>(noteNumber - amountToShiftNote),
            false,
            0);

        region.RemoveEvent(start, noteNumber);
        region.RemoveEvent(start + length, noteNumber);
    }
}

void NotesEditor::RenderNotesCanvas(
    Region &region)
{
    static uint32_t previousNoteWhenDragging = 0;

    auto origin = ImGui::GetCursorScreenPos();

    ImGui::InvisibleButton(
        "##NotesCanvas",
        ImVec2(StepsToPixels(region.Length()), midiEventHeight * (127 - 21)));

    if (!_drawingNotes && ImGui::IsItemHovered() && ImGui::IsMouseDown(0))
    {
        _drawingNotes = true;
        _noteDrawingAndEditingStart = ImGui::GetMousePos();

        auto noteNumber = 127 - std::floor((_noteDrawingAndEditingStart.y - origin.y) / midiEventHeight);
        _notePreviewService.PreviewNote(uint32_t(noteNumber), 100, 10);

        return;
    }

    if (_drawingNotes && ImGui::IsItemActive())
    {
        auto noteToCreate = 127 - std::floor((ImGui::GetMousePos().y - origin.y) / midiEventHeight);

        if (uint32_t(noteToCreate) != previousNoteWhenDragging)
        {
            _notePreviewService.PreviewNote(noteToCreate, 100, 100);

            previousNoteWhenDragging = uint32_t(noteToCreate);
        }

        // Render the preview of the note that wil be created
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(
                _noteDrawingAndEditingStart.x,
                1 + origin.y + ((127 - noteToCreate) * midiEventHeight)),
            ImVec2(
                ImGui::GetMousePos().x,
                origin.y + ((127 - noteToCreate) * midiEventHeight) + midiEventHeight),
            ImColor(ImGui::GetStyle().Colors[ImGuiCol_Button]));

        return;
    }

    if (_drawingNotes && ImGui::IsMouseReleased(0))
    {
        _drawingNotes = false;
        auto noteToCreate = 127 - std::floor((_noteDrawingAndEditingStart.y - origin.y) / midiEventHeight);
        auto noteStart = SnapNotesSteps(PixelsToSteps(_noteDrawingAndEditingStart.x - origin.x));
        auto noteEnd = SnapNotesSteps(PixelsToSteps(ImGui::GetMousePos().x - origin.x));

        if (noteStart > noteEnd)
        {
            auto tmp = noteStart;
            noteStart = noteEnd;
            noteEnd = tmp;
        }

        if ((noteEnd - noteStart) < 1000)
        {
            noteEnd = noteStart + std::chrono::milliseconds::rep(1000);
        }

        region.AddEvent(noteStart, static_cast<uint32_t>(noteToCreate), true, 100);
        region.AddEvent(noteEnd, static_cast<uint32_t>(noteToCreate), false, 0);
    }
}

void NotesEditor::HandleNotesEditorShortCuts()
{
    if (ImGui::GetIO().KeyCtrl)
    {
        _pixelsPerStep += int(ImGui::GetIO().MouseWheel);
        if (_pixelsPerStep < minPixelsPerStep)
        {
            _pixelsPerStep = minPixelsPerStep;
        }
        else if (_pixelsPerStep > maxPixelsPerStep)
        {
            _pixelsPerStep = maxPixelsPerStep;
        }
    }
}
