#include "noteseditor.h"

#include "IconsFontaudio.h"
#include "IconsForkAwesome.h"
#include "arpeggiatorpreviewservice.h"
#include "imguiutils.h"
#include "midinote.h"
#include "notepreviewservice.h"
#include "pianowindow.h"
#include "region.h"
#include "track.h"

#include <algorithm>
#include <imgui_internal.h>
#include <iostream>
#include <spdlog/spdlog.h>

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
            ImGui::Separator();
            ImGui::SameLine();

            ImGui::PushItemWidth(100);
            ImGui::Text("zoom H :");
            ImGui::SameLine();
            ImGui::SliderInt("##zoom", &(_pixelsPerStep), minPixelsPerStep, maxPixelsPerStep);
            ImGui::PopItemWidth();

            ImGui::SameLine();
            ImGui::Separator();
            ImGui::SameLine();

            int &selectedArp = (int &)_arpeggiatorPreviewService.CurrentArpeggiator.Mode;
            int &rate = (int &)_arpeggiatorPreviewService.CurrentArpeggiator.Rate;

            if (ImGui::Button("Open Arpeggiator"))
            {
                ImGui::OpenPopup("Arpeggiator");
            }
            if (ImGui::BeginPopupModal("Arpeggiator", nullptr, ImGuiWindowFlags_NoResize))
            {
                bool previewEnabled = _arpeggiatorPreviewService.Enabled;
                if (ImGui::Checkbox("Preview", &previewEnabled))
                {
                    _arpeggiatorPreviewService.SetEnabled(previewEnabled);
                }

                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 5));

                ImGui::Separator();

                ImGui::AlignTextToFramePadding();
                ImGui::Text("Arp Mode:");
                ImGui::SameLine(90);
                ImGui::RadioIconButton(ICON_FAD_ARPPLAYORDER, "Play Order", 0, &selectedArp);
                ImGui::SameLine();
                ImGui::RadioIconButton(ICON_FAD_ARPCHORD, "Chord", 1, &selectedArp);
                ImGui::SameLine();
                ImGui::RadioIconButton(ICON_FAD_ARPDOWN, "Down", 2, &selectedArp);
                ImGui::SameLine();
                ImGui::RadioIconButton(ICON_FAD_ARPDOWNANDUP, "Down And up", 3, &selectedArp);
                ImGui::SameLine();
                ImGui::RadioIconButton(ICON_FAD_ARPDOWNUP, "Down Up", 4, &selectedArp);
                ImGui::SameLine();
                ImGui::RadioIconButton(ICON_FAD_ARPUP, "Up", 5, &selectedArp);
                ImGui::SameLine();
                ImGui::RadioIconButton(ICON_FAD_ARPUPANDOWN, "Up And Down", 6, &selectedArp);
                ImGui::SameLine();
                ImGui::RadioIconButton(ICON_FAD_ARPUPDOWN, "Up Down", 7, &selectedArp);
                ImGui::SameLine();
                ImGui::RadioIconButton(ICON_FAD_ARPRANDOM, "Random", 8, &selectedArp);

                ImGui::Separator();

                ImGui::AlignTextToFramePadding();
                ImGui::Text("Rate:");
                ImGui::SameLine(90);
                ImGui::RadioIconButton("1/4", "1/4 notes", 4, &rate);
                ImGui::SameLine();
                ImGui::RadioIconButton("1/8", "1/8 notes", 8, &rate);
                ImGui::SameLine();
                ImGui::RadioIconButton("1/16", "1/16 notes", 16, &rate);
                ImGui::SameLine();
                ImGui::RadioIconButton("1/32", "1/32 notes", 32, &rate);

                ImGui::PopStyleVar();

                ImGui::Separator();

                ImGui::Knob("Length", &(_arpeggiatorPreviewService.CurrentArpeggiator.Length), 0.0f, 1.0f, ImVec2(55, 55));

                ImGui::SameLine();

                ImGui::BeginGroup();
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 5));
                for (size_t i = 0; i < 16; i++)
                {
                    ImGui::PushID(static_cast<int>(i));
                    if (i > 0) ImGui::SameLine();
                    if (i < _arpeggiatorPreviewService.CurrentArpeggiator.Notes.size())
                    {
                        auto &note = _arpeggiatorPreviewService.CurrentArpeggiator.Notes[i];
                        ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor::HSV(note.Note / float(12 * 3), 0.6f, 0.6f));
                        ImGui::VSliderInt("##velocity", ImVec2(30, 100), (int *)&(note.Velocity), 0, 128);
                        ImGui::PopStyleColor();
                    }
                    else
                    {
                        ImGui::InvisibleButton("##velocity", ImVec2(30, 100));
                    }
                    ImGui::PopID();
                }

                char label[8] = {0};
                for (size_t i = 0; i < 16; i++)
                {
                    sprintf_s(label, 8, "%zu", i + 1);
                    if (i > 0) ImGui::SameLine();

                    if (i < _arpeggiatorPreviewService.CurrentArpeggiator.Notes.size())
                    {
                        auto &note = _arpeggiatorPreviewService.CurrentArpeggiator.Notes[i];
                        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(note.Note / float(12 * 3), 0.6f, 0.6f));
                    }

                    ImGui::Button(label, ImVec2(30, 30));
                    if (i < _arpeggiatorPreviewService.CurrentArpeggiator.Notes.size())
                    {
                        ImGui::PopStyleColor();
                    }
                }
                ImGui::PopStyleVar();
                ImGui::EndGroup();
                ImGui::SameLine();

                ImGui::BeginGroup();
                if (ImGui::Button("Delete last"))
                {
                    _arpeggiatorPreviewService.CurrentArpeggiator.Notes.pop_back();
                }
                if (ImGui::Button("Clear"))
                {
                    _arpeggiatorPreviewService.CurrentArpeggiator.Notes.clear();
                }
                ImGui::EndGroup();

                ImGui::Separator();

                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 2));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 18));

                const int keyWidth = 28;
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.05f, 0.18f, 0.3f, 1.0f));
                ImGui::InvisibleButton("spacer", ImVec2(keyWidth / 2, 40));
                ImGui::PushID("ArpKeys");
                for (int i = 3; i < 6; i++)
                {
                    ImGui::SameLine();
                    NoteButton(firstKeyNoteNumber + (i * 12) + Note_CSharp_OffsetFromC, ImVec2(keyWidth, 40));
                    ImGui::SameLine();
                    NoteButton(firstKeyNoteNumber + (i * 12) + Note_DSharp_OffsetFromC, ImVec2(keyWidth, 40));
                    ImGui::SameLine();
                    ImGui::InvisibleButton("spacer", ImVec2(keyWidth, 40));
                    ImGui::SameLine();
                    NoteButton(firstKeyNoteNumber + (i * 12) + Note_FSharp_OffsetFromC, ImVec2(keyWidth, 40));
                    ImGui::SameLine();
                    NoteButton(firstKeyNoteNumber + (i * 12) + Note_GSharp_OffsetFromC, ImVec2(keyWidth, 40));
                    ImGui::SameLine();
                    NoteButton(firstKeyNoteNumber + (i * 12) + Note_ASharp_OffsetFromC, ImVec2(keyWidth, 40));
                    ImGui::SameLine();
                    ImGui::InvisibleButton("spacer", ImVec2(keyWidth, 40));
                }
                ImGui::Dummy(ImVec2(0, 0));
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                for (int i = 3; i < 6; i++)
                {
                    NoteButton(firstKeyNoteNumber + (i * 12) + Note_C_OffsetFromC, ImVec2(keyWidth, 40));
                    ImGui::SameLine();
                    NoteButton(firstKeyNoteNumber + (i * 12) + Note_D_OffsetFromC, ImVec2(keyWidth, 40));
                    ImGui::SameLine();
                    NoteButton(firstKeyNoteNumber + (i * 12) + Note_E_OffsetFromC, ImVec2(keyWidth, 40));
                    ImGui::SameLine();
                    NoteButton(firstKeyNoteNumber + (i * 12) + Note_F_OffsetFromC, ImVec2(keyWidth, 40));
                    ImGui::SameLine();
                    NoteButton(firstKeyNoteNumber + (i * 12) + Note_G_OffsetFromC, ImVec2(keyWidth, 40));
                    ImGui::SameLine();
                    NoteButton(firstKeyNoteNumber + (i * 12) + Note_A_OffsetFromC, ImVec2(keyWidth, 40));
                    ImGui::SameLine();
                    NoteButton(firstKeyNoteNumber + (i * 12) + Note_B_OffsetFromC, ImVec2(keyWidth, 40));
                    ImGui::SameLine();
                }
                ImGui::PopID();
                ImGui::PopStyleColor(2);
                ImGui::Dummy(ImVec2(0, 0));

                ImGui::PopStyleVar(2);

                ImGui::Separator();

                if (ImGui::Button("Replace", ImVec2(120, 0)))
                {
                    _arpeggiatorPreviewService.Enabled = false;
                    KillAllNotes();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SetItemDefaultFocus();
                ImGui::SameLine();
                if (ImGui::Button("Append", ImVec2(120, 0)))
                {
                    _arpeggiatorPreviewService.Enabled = false;
                    KillAllNotes();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 0)))
                {
                    _arpeggiatorPreviewService.Enabled = false;
                    KillAllNotes();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
        ImGui::EndChild();

        auto trackId = std::get<uint32_t>(_tracks->GetActiveRegion());
        auto regionStart = std::get<std::chrono::milliseconds::rep>(_tracks->GetActiveRegion());

        if (trackId == Track::Null || regionStart < 0)
        {
            ImGui::Text("No region selected");
        }
        else
        {
            auto &track = _tracks->GetTrack(trackId);

            float _tracksScrollx = 0;

            auto &region = track.GetRegion(regionStart);
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

        if (_lastNotePreviewY != diffy)
        {
            auto amountToShiftNote = std::floor(diffy / float(midiEventHeight));

            _notePreviewService.PreviewNote(uint32_t(noteNumber - amountToShiftNote), 100, length);

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
