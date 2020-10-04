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

const int midiEventHeight = 10;
const int minPixelsPerStep = 8;
const int maxPixelsPerStep = 200;

NotesEditor::NotesEditor()
{
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
            ImVec2(0, trackToolsHeight));
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
            auto notes = MidiNote::ConvertMidiEventsToMidiNotes(region._events);

            auto originContainerScreenPos = ImGui::GetCursorScreenPos();

            ImGui::MoveCursorPos(ImVec2(0, timelineHeight));

            ImGui::BeginChild(
                "NotesContainer",
                ImVec2(0, 0),
                false,
                ImGuiWindowFlags_HorizontalScrollbar);
            {
                auto origin = ImGui::GetCursorPos();

                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 1));

                ImGui::BeginGroup();
                for (int noteNumber = 127; noteNumber >= 0; noteNumber--)
                {
                    auto originNotePos = ImGui::GetCursorPos();
                    auto originNoteScreenPos = ImGui::GetCursorScreenPos();

                    auto gridWidth = std::max(StepsToPixels(region._length), ImGui::GetWindowWidth());
                    auto noteInOctave = noteNumber % 12;
                    if (noteInOctave == Note_C_OffsetFromC || noteInOctave == Note_D_OffsetFromC || noteInOctave == Note_E_OffsetFromC || noteInOctave == Note_F_OffsetFromC || noteInOctave == Note_G_OffsetFromC || noteInOctave == Note_A_OffsetFromC || noteInOctave == Note_B_OffsetFromC)
                    {
                        ImGui::GetWindowDrawList()->AddRectFilled(
                            ImVec2(originNoteScreenPos.x, originNoteScreenPos.y + 1),
                            ImVec2(originNoteScreenPos.x + gridWidth, originNoteScreenPos.y + midiEventHeight),
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
                                ImVec2(StepsToPixels(notesInTime.first), originNotePos.y + 1));

                            RenderEditableNote(
                                region,
                                noteNumber,
                                notesInTime.first,
                                note.length,
                                note.velocity,
                                ImVec2(StepsToPixels(note.length), midiEventHeight - 1),
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
                StepsToPixels(region._length),
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
    unsigned int velocity,
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
        region.AddEvent(start + PixelsToSteps(ImGui::GetMousePos().x - _noteDrawingAndEditingStart.x), noteNumber - amountToShiftNote, true, velocity);
        region.AddEvent(start + PixelsToSteps(ImGui::GetMousePos().x - _noteDrawingAndEditingStart.x) + length, noteNumber - amountToShiftNote, false, 0);

        region.RemoveEvent(start, noteNumber);
        region.RemoveEvent(start + length, noteNumber);
    }
}

void NotesEditor::RenderNotesCanvas(
    Region &region,
    const ImVec2 &origin)
{
    ImGui::InvisibleButton(
        "##NotesCanvas",
        ImVec2(StepsToPixels(region._length), midiEventHeight * 127));

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

        region.AddEvent(noteStart, noteToCreate, true, 100);
        region.AddEvent(noteEnd, noteToCreate, false, 0);
    }

    if (ImGui::IsItemActive() && _drawingNotes)
    {
        auto noteToCreate = std::floor((_noteDrawingAndEditingStart.y - origin.y - timelineHeight - ImGui::GetScrollY()) / midiEventHeight);
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(_noteDrawingAndEditingStart.x, 1 + origin.y + timelineHeight + ImGui::GetScrollY() + (noteToCreate * midiEventHeight)),
            ImVec2(ImGui::GetMousePos().x, origin.y + timelineHeight + ImGui::GetScrollY() + (noteToCreate * midiEventHeight) + midiEventHeight),
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
