#include "noteseditor.h"

#include "IconsFontaudio.h"
#include "IconsForkAwesome.h"
#include "imguiutils.h"
#include "midinote.h"
#include "region.h"
#include "track.h"

#include <algorithm>
#include <imgui_internal.h>
#include <iostream>

const int midiEventHeight = 12;
const int minPixelsPerStep = 8;
const int maxPixelsPerStep = 200;

NotesEditor::NotesEditor() = default;

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
            ImGui::VerticalSeparator();
            ImGui::SameLine();

            ImGui::PushItemWidth(100);
            ImGui::Text("zoom H :");
            ImGui::SameLine();
            ImGui::SliderInt("##zoom", &(_pixelsPerStep), minPixelsPerStep, maxPixelsPerStep);
            ImGui::PopItemWidth();

            ImGui::SameLine();
            ImGui::VerticalSeparator();
            ImGui::SameLine();

            if (ImGui::Button("Open Arpeggiator"))
            {
                ImGui::OpenPopup("Arpeggiator");
            }

            if (ImGui::BeginPopupModal("Arpeggiator"))
            {
                ImGui::Text("Aquarium");
                ImGui::Separator();
                ImGui::Text("s sdf");

                if (ImGui::Button("Replace", ImVec2(120,0))) { ImGui::CloseCurrentPopup(); }
                ImGui::SetItemDefaultFocus();
                ImGui::SameLine();
                if (ImGui::Button("Append", ImVec2(120,0))) { ImGui::CloseCurrentPopup(); }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120,0))) { ImGui::CloseCurrentPopup(); }
                ImGui::EndPopup();
            }
        }
        ImGui::EndChild();

        auto track = std::get<ITrack *>(_tracks->GetActiveRegion());
        auto regionStart = std::get<long>(_tracks->GetActiveRegion());

        if (track == nullptr || regionStart < 0)
        {
            ImGui::Text("No region selected");
        }
        else
        {
            float _tracksScrollx = 0;

            auto &region = track->GetRegion(regionStart);
            auto notes = MidiNote::ConvertMidiEventsToMidiNotes(region.Events());

            auto originContainerScreenPos = ImGui::GetCursorScreenPos();

            ImGui::MoveCursorPos(ImVec2(0.0f, float(timelineHeight)));

            ImGui::BeginChild(
                "NotesContainer",
                ImVec2(0, 0),
                false,
                ImGuiWindowFlags_HorizontalScrollbar);
            {
                auto origin = ImGui::GetCursorPos();

                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 1));

                ImGui::BeginGroup();
                long eindex = 0;
                static ImGuiID movingEventId;
                for (auto t : region.Events())
                {
                    for (auto e : t.second)
                    {
                        ImGui::PushID(eindex++);

                        auto x = StepsToPixels(t.first);
                        auto y = (127 - e.num) * midiEventHeight + 1;
                        ImGui::SetCursorPos(ImVec2(origin.x + x - (midiEventHeight / 2), origin.y + y));

                        if (ImGui::ButtonEx("event", ImVec2(midiEventHeight, midiEventHeight - 1), ImGuiButtonFlags_PressedOnClick))
                        {
                            movingEventId = ImGui::GetID("event");
                        }

                        if (ImGui::IsItemHovered())
                        {
                            if (e.type == MidiEventTypes::M_NOTE && e.value > 0)
                            {
                                ImGui::SetTooltip("Move Note Down event");
                            }
                            if (e.type == MidiEventTypes::M_NOTE && e.value <= 0)
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
                            auto move = ImGui::GetIO().MousePos.x - ImGui::GetIO().MouseClickedPos[0].x;
                            region.MoveEvent(
                                e,
                                t.first,
                                t.first + PixelsToSteps(move));
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

                ImGui::SetCursorPos(origin);

                for (int noteNumber = 127; noteNumber >= 0; noteNumber--)
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
                    for (auto notesInTime : notes[noteNumber])
                    {
                        for (auto note : notesInTime.second)
                        {
                            ImGui::PushID(note.length);

                            ImGui::SetCursorPos(
                                ImVec2(StepsToPixels(notesInTime.first) + (midiEventHeight * 0.5f), originNotePos.y + 1));

                            RenderEditableNote(
                                region,
                                noteNumber,
                                notesInTime.first,
                                note.length,
                                note.velocity,
                                ImVec2(StepsToPixels(note.length) - midiEventHeight, midiEventHeight - 1),
                                originContainerScreenPos);

                            ImGui::PopID();
                        }
                    }
                    ImGui::PopID();

                    ImGui::SetCursorPos(ImVec2(originNotePos.x, originNotePos.y + midiEventHeight));
                }

                ImGui::EndGroup();

                ImGui::PopStyleVar();

                ImGui::SetCursorPos(origin);

                RenderNotesCanvas(region, originContainerScreenPos);

                _tracksScrollx = ImGui::GetScrollX();
            }
            ImGui::EndChild();

            RenderTimeline(
                ImVec2(originContainerScreenPos.x, originContainerScreenPos.y),
                StepsToPixels(region.Length()),
                _tracksScrollx);

            RenderCursor(
                ImVec2(originContainerScreenPos.x, originContainerScreenPos.y),
                size,
                _tracksScrollx,
                0);
        }
    }
    ImGui::End();

    HandleNotesEditorShortCuts();
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

    auto buttonPos = ImGui::GetCursorScreenPos();
    if (ImGui::ButtonEx("note", noteSize, ImGuiButtonFlags_PressedOnClick))
    {
        movingNoteId = ImGui::GetID("note");
        if (ImGui::GetIO().KeyShift)
        {
            // add/remove note to/from selection
        }
        else
        {
            // reset selection and make only this note the only selected note
        }
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Move whole Note");
    }
    auto diffy = ImGui::GetMousePos().y - _noteDrawingAndEditingStart.y;
    diffy -= (int(std::floor(diffy)) % midiEventHeight);

    if (ImGui::IsItemActive() && _editingNotes)
    {
        auto noteStart = ImVec2(
            buttonPos.x + ImGui::GetMousePos().x - _noteDrawingAndEditingStart.x,
            buttonPos.y + diffy);

        ImGui::GetWindowDrawList()->AddRectFilled(
            noteStart,
            ImVec2(noteStart.x + noteSize.x, noteStart.y + noteSize.y),
            ImColor(ImGui::GetStyle().Colors[ImGuiCol_PlotHistogram]));
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
    else if (ImGui::GetID("note") == movingNoteId && _editingNotes && ImGui::IsMouseReleased(0))
    {
        _editingNotes = false;
        auto amountToShiftNote = std::floor(diffy / float(midiEventHeight));

        auto from = start + PixelsToSteps(ImGui::GetMousePos().x - _noteDrawingAndEditingStart.x);

        region.AddEvent(
            from,
            noteNumber - amountToShiftNote,
            true,
            velocity);

        region.AddEvent(
            from + length,
            noteNumber - amountToShiftNote,
            false,
            0);

        region.RemoveEvent(start, noteNumber);
        region.RemoveEvent(start + length, noteNumber);
    }
}

void NotesEditor::RenderNotesCanvas(
    Region &region,
    const ImVec2 &origin)
{
    static uint32_t previousNoteWhenDragging = 0;

    ImGui::InvisibleButton(
        "##NotesCanvas",
        ImVec2(StepsToPixels(region.Length()), midiEventHeight * 127));

    if (ImGui::IsItemHovered() && ImGui::IsMouseDown(0) && !_drawingNotes)
    {
        _drawingNotes = true;
        _noteDrawingAndEditingStart = ImGui::GetMousePos();
    }
    else if (_drawingNotes && ImGui::IsMouseReleased(0))
    {
        _drawingNotes = false;
        auto noteToCreate = 127 - std::floor((_noteDrawingAndEditingStart.y - origin.y - timelineHeight + ImGui::GetScrollY()) / midiEventHeight);
        auto noteStart = PixelsToSteps(_noteDrawingAndEditingStart.x - origin.x);
        auto noteEnd = PixelsToSteps(ImGui::GetMousePos().x - origin.x);

        if (noteStart > noteEnd)
        {
            auto tmp = noteStart;
            noteStart = noteEnd;
            noteEnd = tmp;
        }
        region.AddEvent(noteStart, noteToCreate, true, 100);
        region.AddEvent(noteEnd, noteToCreate, false, 0);
    }

    if (ImGui::IsItemActive() && _drawingNotes)
    {
        auto noteToCreate = std::floor((_noteDrawingAndEditingStart.y - origin.y - timelineHeight - ImGui::GetScrollY()) / midiEventHeight);
        if (uint32_t(noteToCreate) != previousNoteWhenDragging)
        {
            // todo play note with a length from the selected note

            previousNoteWhenDragging = uint32_t(noteToCreate);
        }

        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(
                _noteDrawingAndEditingStart.x,
                1 + origin.y + timelineHeight + ImGui::GetScrollY() + (noteToCreate * midiEventHeight)),
            ImVec2(
                ImGui::GetMousePos().x,
                origin.y + timelineHeight + ImGui::GetScrollY() + (noteToCreate * midiEventHeight) + midiEventHeight),
            ImColor(ImGui::GetStyle().Colors[ImGuiCol_Button]));
    }
}

void NotesEditor::HandleNotesEditorShortCuts()
{
    if (ImGui::GetIO().KeyCtrl)
    {
        _pixelsPerStep += ImGui::GetIO().MouseWheel;
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
