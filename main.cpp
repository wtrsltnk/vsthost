#include <glad/glad.h>

#include <GL/wglext.h>
#include <algorithm>
#include <chrono>
#include <commdlg.h>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <spdlog/spdlog.h>
#include <sstream>
#include <thread>
#include <windows.h>

#include "imgui.h"
#include "imgui_impl_win32_gl2.h"
#include "imgui_internal.h"

#include "IconsFontaudio.h"
#include "IconsForkAwesome.h"
#include "RtError.h"
#include "RtMidi.h"
#include "arpeggiatorpreviewservice.h"
#include "imguiutils.h"
#include "inspectorwindow.h"
#include "instrument.h"
#include "midicontrollers.h"
#include "midievent.h"
#include "noteseditor.h"
#include "pianowindow.h"
#include "region.h"
#include "state.h"
#include "track.h"
#include "trackseditor.h"
#include "tracksmanager.h"
#include "tracksserializer.h"
#include "vstplugin.h"
#include "wasapi.h"
#include "win32vstpluginservice.h"
#include <ImGuiFileDialog.h>
#include <ImGuiFileDialog.h>

static std::map<int, bool> _noteStates;
static std::map<int, struct MidiNoteState> _keyboardToNoteMap{
    {'Z', MidiNoteState(60)}, // C-4
    {'S', MidiNoteState(61)}, // C#4
    {'X', MidiNoteState(62)}, // D-4
    {'D', MidiNoteState(63)}, // D#4
    {'C', MidiNoteState(64)}, // E-4
    {'V', MidiNoteState(65)}, // F-4
    {'G', MidiNoteState(66)}, // F#4
    {'B', MidiNoteState(67)}, // G-4
    {'H', MidiNoteState(68)}, // G#4
    {'N', MidiNoteState(69)}, // A-4
    {'J', MidiNoteState(70)}, // A#4
    {'M', MidiNoteState(71)}, // B-4

    {'Q', MidiNoteState(72)}, // C-5
    {'2', MidiNoteState(73)}, // C#5
    {'W', MidiNoteState(74)}, // D-5
    {'3', MidiNoteState(75)}, // D#5
    {'E', MidiNoteState(76)}, // E-5
    {'R', MidiNoteState(77)}, // F-5
    {'5', MidiNoteState(78)}, // F#5
    {'T', MidiNoteState(79)}, // G-5
    {'6', MidiNoteState(80)}, // G#5
    {'Y', MidiNoteState(81)}, // A-5
    {'7', MidiNoteState(82)}, // A#5
    {'U', MidiNoteState(83)}, // B-5

    {'I', MidiNoteState(84)}, // C-6
    {'9', MidiNoteState(85)}, // C#6
    {'O', MidiNoteState(86)}, // D-6
    {'0', MidiNoteState(87)}, // D#6
    {'P', MidiNoteState(88)}, // E-6
};
static RtMidiIn *midiIn = nullptr;
static State state;
static TracksManager _tracks;
static GLFWwindow *window = nullptr;
static ImVec4 clear_color = ImVec4(0.3f, 0.3f, 0.3f, 1.00f);
static TracksEditor _tracksEditor;
static NotesEditor _notesEditor;
static InspectorWindow _inspectorWindow;
static PianoWindow _pianoWindow;
static Win32VstPluginService *_vstPluginService = nullptr;
static bool _showInspectorWindow = true;
static bool _showPianoWindow = true;
ArpeggiatorPreviewService _arpeggiatorPreviewService;

void KillAllNotes()
{
    for (auto &instrument : _tracks.GetInstruments())
    {
        for (int channel = 0; channel < 16; channel++)
        {
            for (int note = 0; note < 128; note++)
            {
                instrument->Plugin()
                    ->sendMidiNote(
                        channel,
                        note,
                        false,
                        0);
            }
        }
    }
}

void HandleIncomingMidiEvent(
    int midiChannel,
    int noteNumber,
    bool onOff,
    int velocity);

// This function is called from Wasapi::threadFunc() which is running in audio thread.
bool refillCallback(
    ITracksManager *tracks,
    float *const data,
    uint32_t sampleCount,
    const WAVEFORMATEX *const mixFormat)
{
    auto start = state._cursor;
    auto diff = sampleCount * (1000.0 / mixFormat->nSamplesPerSec);
    state.UpdateByDiff(diff);
    auto end = state._cursor;

    tracks->SendMidiNotesInSong(start, end);
    _arpeggiatorPreviewService.SendMidiNotesInTimeRange(diff);

    const auto nDstChannels = mixFormat->nChannels;

    for (size_t iFrame = 0; iFrame < sampleCount; ++iFrame)
    {
        for (size_t iChannel = 0; iChannel < nDstChannels; ++iChannel)
        {
            const size_t wasapiWriteIndex = iFrame * nDstChannels + iChannel;
            *(data + wasapiWriteIndex) = 0;
        }
    }

    for (auto track : tracks->GetTracks())
    {
        auto instrument = track->GetInstrument();
        if (instrument == nullptr)
        {
            continue;
        }

        auto vstPlugin = instrument->Plugin();
        if (vstPlugin == nullptr)
        {
            continue;
        }

        vstPlugin->processEvents();

        size_t tmpsampleCount = sampleCount;
        const auto nSrcChannels = vstPlugin->getChannelCount();
        const auto vstSamplesPerBlock = vstPlugin->getBlockSize();

        size_t ofs = 0;
        while (tmpsampleCount > 0)
        {
            size_t outputFrameCount = 0;
            float **vstOutput = vstPlugin->processAudio(tmpsampleCount, outputFrameCount);

            if (vstOutput == nullptr)
            {
                break;
            }

            const auto nFrame = outputFrameCount;
            for (size_t iFrame = 0; iFrame < nFrame; ++iFrame)
            {
                for (size_t iChannel = 0; iChannel < nDstChannels; ++iChannel)
                {
                    if (track->IsMuted()) continue;
                    if (data == nullptr) continue;
                    if (_tracks.GetSoloTrack() != track && _tracks.GetSoloTrack() != nullptr) continue;

                    const size_t sChannel = iChannel % nSrcChannels;
                    const size_t vstOutputPage = (iFrame / vstSamplesPerBlock) * sChannel + sChannel;
                    const size_t vstOutputIndex = (iFrame % vstSamplesPerBlock);
                    const size_t wasapiWriteIndex = iFrame * nDstChannels + iChannel;

                    *(data + ofs + wasapiWriteIndex) += vstOutput[vstOutputPage][vstOutputIndex];
                }
            }

            tmpsampleCount -= nFrame;
            ofs += nFrame * nDstChannels;
        }
    }

    return true;
}

void HandleIncomingMidiEvent(
    int midiChannel,
    int noteNumber,
    bool onOff,
    int velocity)
{
    if (onOff)
    {
        _arpeggiatorPreviewService.TriggerNote(noteNumber, velocity);
        PianoWindow::downKeys.insert(noteNumber);
    }
    else
    {
        PianoWindow::downKeys.erase(noteNumber);
    }

    if (state.IsRecording())
    {
        for (auto &track : _tracks.GetTracks())
        {
            if (!track->IsReadyForRecoding())
            {
                continue;
            }

            track->RecordMidiEvent(
                state._cursor,
                noteNumber,
                onOff,
                velocity);
        }
    }

    for (auto &instrument : _tracks.GetInstruments())
    {
        if (instrument->Plugin() == nullptr)
        {
            continue;
        }

        instrument->Plugin()
            ->sendMidiNote(
                midiChannel,
                noteNumber,
                onOff,
                velocity);
    }
}

void HandleKeyUpDown(
    int noteNumber)
{
    bool keyState = false;

    if (_noteStates.find(noteNumber) != _noteStates.end())
    {
        keyState = _noteStates[noteNumber];
    }

    if (!ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
    {
        if (keyState)
        {
            HandleIncomingMidiEvent(0, noteNumber, false, 0);
            _noteStates[noteNumber] = false;
        }

        return;
    }

    if (ImGui::IsMouseDown(0) && !keyState)
    {
        HandleIncomingMidiEvent(0, noteNumber, true, 100);
        _noteStates[noteNumber] = true;
    }
    else if (ImGui::IsMouseReleased(0) && keyState)
    {
        HandleIncomingMidiEvent(0, noteNumber, false, 0);
        _noteStates[noteNumber] = false;
    }
}

void callback(
    double /*timeStamp*/,
    std::vector<unsigned char> *message,
    void * /*userData*/)
{
    MidiEvent ev;
    unsigned char chan = message->at(0) & 0x0f;
    switch (message->at(0) & 0xf0)
    {
        case 0x80: //Note Off
        {
            ev.type = MidiEventTypes::M_NOTE;
            ev.channel = chan;
            ev.num = message->at(1);
            ev.value = 0;

            HandleIncomingMidiEvent(ev.channel, ev.num, false, ev.value);
            break;
        }
        case 0x90: //Note On
        {
            ev.type = MidiEventTypes::M_NOTE;
            ev.channel = chan;
            ev.num = message->at(1);
            ev.value = message->at(2);

            HandleIncomingMidiEvent(ev.channel, ev.num, true, ev.value);
            break;
        }
        case 0xA0: /* pressure, aftertouch */
        {
            ev.type = MidiEventTypes::M_PRESSURE;
            ev.channel = chan;
            ev.num = message->at(1);
            ev.value = message->at(2);
            break;
        }
        case 0xb0: //Controller
        {
            ev.type = MidiEventTypes::M_CONTROLLER;
            ev.channel = chan;
            ev.num = message->at(1);
            ev.value = message->at(2);
            break;
        }
        case 0xe0: //Pitch Wheel
        {
            ev.type = MidiEventTypes::M_CONTROLLER;
            ev.channel = chan;
            ev.num = MidiControllers::C_pitchwheel;
            ev.value = (message->at(1) + message->at(2) * 128) - 8192;
            break;
        }
        case 0xc0: //Program Change
        {
            ev.type = MidiEventTypes::M_PGMCHANGE;
            ev.channel = chan;
            ev.num = message->at(1);
            ev.value = 0;
            break;
        }
        default:
        {
            return;
        }
    }
}

void ToolbarWindow(
    ImVec2 const &pos,
    ImVec2 const &size)
{
    ImGui::Begin("Toolbar", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
    {
        ImGui::SetWindowPos(pos);
        ImGui::SetWindowSize(size);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

        if (ImGui::Button(ICON_FK_LIST))
        {
            _showInspectorWindow = !_showInspectorWindow;
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(_showInspectorWindow ? "Hide Inspector Window" : "Show Inspector Window");
        }

        ImGui::SameLine();

        if (ImGui::Button(ICON_FAD_KEYBOARD))
        {
            _showPianoWindow = !_showPianoWindow;
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(_showPianoWindow ? "Hide Piano Window" : "Show Piano Window");
        }

        ImGui::PopStyleVar();

        ImGui::SameLine();
        ImGui::Separator();
        ImGui::SameLine();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

        if (ImGui::Button(ICON_FAD_BACKWARD))
        {
        }

        ImGui::SameLine();

        if (ImGui::Button(ICON_FAD_FORWARD))
        {
        }

        ImGui::SameLine();

        if (ImGui::Button(ICON_FAD_STOP))
        {
            state.StopPlaying();
            state._recording = false;
        }

        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, state.IsPlaying() ? ImVec4(0, 0.5f, 0, 1) : ImGui::GetStyle().Colors[ImGuiCol_Button]);

        if (ImGui::Button(ICON_FAD_PLAY))
        {
            state.TogglePlaying();
        }

        ImGui::PopStyleColor();

        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, state._recording ? ImVec4(1, 0, 0, 1) : ImGui::GetStyle().Colors[ImGuiCol_Button]);

        if (ImGui::Button(ICON_FAD_RECORD) && !state.IsRecording())
        {
            for (auto track : _tracks.GetTracks())
            {
                track->StartRecording();
            }

            state.StartRecording();
        }

        ImGui::PopStyleColor();

        ImGui::SameLine();

        if (ImGui::ActiveButton(ICON_FAD_LOOP, state._loop))
        {
            state._loop = !state._loop;
        }

        ImGui::PopStyleVar();

        ImGui::SameLine();
        ImGui::Separator();
        ImGui::SameLine();

        ImGui::Text("bpm :");
        ImGui::SameLine();

        ImGui::PushItemWidth(100);
        ImGui::SliderInt("##bpm", (int *)&(state._bpm), 10, 200);
        ImGui::PopItemWidth();

        ImGui::SameLine();
        ImGui::Separator();
        ImGui::SameLine();

        if (ImGui::Button(ICON_FK_PLUS))
        {
            _tracks.SetActiveTrack(_tracks.AddVstTrack());
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Add Track");
        }
    }
    ImGui::End();
}

static int toolbarHeight = 62;
const int pianoHeight = 180;
const int inspectorWidth = 350;

void StyleColorsCustomDark(ImGuiStyle *dst = nullptr)
{
    ImGui::StyleColorsDark(dst);

    ImGuiStyle *style = dst ? dst : &ImGui::GetStyle();
    ImVec4 *colors = style->Colors;

    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.10f, 0.12f, 0.94f);
    //    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    //    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    //    colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
    //    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    //    colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    //    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    //    colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
    //    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    //    colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    //    colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    //    colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
    //    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    //    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    //    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    //    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    //    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    //    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    //    colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    //    colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    //    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    //    colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    //    colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    //    colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    //    colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    //    colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    //    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    //    colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
    //    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    //    colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    //    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    //    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    //    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    //    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    //    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    //    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    //    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    //    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    //    colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    //    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    //    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    //    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
}

void MainMenu()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New", "CTRL+N"))
            {
                state.StopPlaying();
                state.StopRecording();
            }

            if (ImGui::MenuItem("Open...", "CTRL+O"))
            {
                state.StopPlaying();
                state.StopRecording();
                igfd::ImGuiFileDialog::Instance()->OpenDialog("OpenFileDlgKey", "Choose File", ".yaml", ".");

                TracksSerializer serializer(_tracks);

                serializer.Deserialize("c:\\temp\\file.yaml");
            }

            if (ImGui::MenuItem("Save", "CTRL+S"))
            {
                state.StopPlaying();
                state.StopRecording();

                TracksSerializer serializer(_tracks);

                serializer.Serialize("c:\\temp\\file.yaml");
            }

            if (ImGui::MenuItem("Save As...", "CTRL+SHIFT+S"))
            {
                state.StopPlaying();
                state.StopRecording();
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Quit", "CTRL+Q"))
            {
                state.StopPlaying();
                state.StopRecording();

                PostQuitMessage(0);
            }

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            if (ImGui::MenuItem("Undo", "CTRL+Z"))
            {
            }
            if (ImGui::MenuItem("Redo", "CTRL+Y", false, false))
            {
            } // Disabled item
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "CTRL+X"))
            {
            }
            if (ImGui::MenuItem("Copy", "CTRL+C"))
            {
            }
            if (ImGui::MenuItem("Paste", "CTRL+V"))
            {
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            if (ImGui::MenuItem("Dark-mode"))
            {
                StyleColorsCustomDark();
            }

            if (ImGui::MenuItem("Light-mode"))
            {
                ImGui::StyleColorsLight();
            }

            if (ImGui::MenuItem("Classic-mode"))
            {
                ImGui::StyleColorsClassic();
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    if (igfd::ImGuiFileDialog::Instance()->FileDialog("OpenFileDlgKey"))
    {
        // action if OK
        if (igfd::ImGuiFileDialog::Instance()->IsOk == true)
        {
            std::string filePathName = igfd::ImGuiFileDialog::Instance()->GetFilePathName();
            std::string filePath = igfd::ImGuiFileDialog::Instance()->GetCurrentPath();
            // action
        }
        // close
        igfd::ImGuiFileDialog::Instance()->CloseDialog("OpenFileDlgKey");
    }
}

void HandleKeyboardToMidiEvents()
{
    if (_tracks.GetInstruments().size() > 0 && _tracksEditor.EditTrackName() < 0)
    {
        for (auto &e : _keyboardToNoteMap)
        {
            auto &key = e.second;
            const auto on = (GetKeyState(e.first) & 0x8000) != 0;
            if (key.status != on)
            {
                key.status = on;
                HandleIncomingMidiEvent(0, key.midiNote, on, 100);
            }
        }
    }
}

void MainLoop()
{
    Wasapi wasapi([&](float *const data, uint32_t availableFrameCount, const WAVEFORMATEX *const mixFormat) {
        return refillCallback(&_tracks, data, availableFrameCount, mixFormat);
    });

    _inspectorWindow.SetAudioOut(&wasapi);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        if (glfwPollEvents())
        {
            break;
        }

        if (glfwGetWindowAttrib(window, GLFW_FOCUSED))
        {
            HandleKeyboardToMidiEvents();
        }

        ImGui_ImplGlfwGL2_NewFrame();

        glfwGetWindowSize(window, &(state.ui._width), &(state.ui._height));

        int currentInspectorWidth = _showInspectorWindow ? inspectorWidth : 0;
        int currentPianoHeight = _showPianoWindow ? pianoHeight : 0;

        auto &style = ImGui::GetStyle();
        ImVec2 currentPos;

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::GetStyle().Colors[ImGuiCol_MenuBarBg]);
        {
            MainMenu();

            toolbarHeight = (style.FramePadding.y * 2) + style.WindowPadding.y + ImGui::GetFont()->FontSize;

            currentPos = ImGui::GetCursorPos();

            ToolbarWindow(
                ImVec2(0, currentPos.y - style.WindowPadding.y),
                ImVec2(state.ui._width, toolbarHeight + style.WindowPadding.y));
        }
        ImGui::PopStyleColor();

        if (_showInspectorWindow)
        {
            _inspectorWindow.Render(
                ImVec2(0, currentPos.y + toolbarHeight),
                ImVec2(currentInspectorWidth, state.ui._height - toolbarHeight));
        }

        if (state.ui._activeCenterScreen == 0)
        {
            _tracksEditor.Render(
                ImVec2(currentInspectorWidth, currentPos.y + toolbarHeight),
                ImVec2(state.ui._width - currentInspectorWidth, state.ui._height - currentPos.y - toolbarHeight - currentPianoHeight));
        }
        else
        {
            _notesEditor.Render(
                ImVec2(currentInspectorWidth, currentPos.y + toolbarHeight),
                ImVec2(state.ui._width - currentInspectorWidth, state.ui._height - currentPos.y - toolbarHeight - currentPianoHeight));
        }

        if (_showPianoWindow)
        {
            _pianoWindow.Render(
                ImVec2(currentInspectorWidth, state.ui._height - pianoHeight),
                ImVec2(state.ui._width - currentInspectorWidth, pianoHeight));
        }

        ImGui::ShowDemoWindow();

        // Rendering
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Render();
        ImGui_ImplGlfwGL2_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    _inspectorWindow.SetAudioOut(nullptr);
}

void SetupFonts()
{
    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->Clear();

    ImFont *font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    if (font != nullptr)
    {
        io.FontDefault = font;
    }
    else
    {
        io.Fonts->AddFontDefault();
    }

    ImFontConfig config;
    config.MergeMode = true;

    static const ImWchar fontaudio_icon_ranges[] = {ICON_MIN_FAD, ICON_MAX_FAD, 0};
    io.Fonts->AddFontFromFileTTF("fonts/fontaudio.ttf", 13.0f, &config, fontaudio_icon_ranges);

    static const ImWchar forkawesome_icon_ranges[] = {ICON_MIN_FK, ICON_MAX_FK, 0};
    io.Fonts->AddFontFromFileTTF("fonts/forkawesome-webfont.ttf", 13.0f, &config, forkawesome_icon_ranges);
    io.Fonts->Build();
}

int main(
    int argc,
    char **argv)
{
    spdlog::set_level(spdlog::level::debug);

    (void)argc;
    (void)argv;

#ifdef TEST_YOUR_CODE
    State::Tests();
#endif

    if (!glfwInit())
    {
        return 1;
    }

    RtMidiIn localMidiIn;
    midiIn = &localMidiIn;
    midiIn->setCallback(callback);

    if (localMidiIn.getPortCount() > 0)
    {
        localMidiIn.openPort(0);
        _showPianoWindow = false;
    }

    window = glfwCreateWindow(1280, 720, "VstHost", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    _vstPluginService = new Win32VstPluginService(window->hwnd);

    // Setup ImGui binding
    ImGui::CreateContext();

    ImGui_ImplGlfwGL2_Init(window, true);

    // Setup style
    StyleColorsCustomDark();
    auto &style = ImGui::GetStyle();

    style.WindowRounding = 0;
    style.WindowBorderSize = 0;
    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(12, 6);
    style.ItemInnerSpacing = ImVec2(20, 20);
    style.ItemSpacing = ImVec2(10, 15);
    style.ChildRounding = 0;
    style.ScrollbarRounding = 0;

    SetupFonts();

    auto bbcSynthTrack = _tracks.AddVstTrack("BBC Symphony Orchestra (64 Bit).dll");
    bbcSynthTrack->AddRegion(0, Region{});
    bbcSynthTrack->SetReadyForRecording(true);
    _tracks.SetActiveTrack(bbcSynthTrack);

    _tracksEditor.SetState(&state);
    _tracksEditor.SetTracksManager(&_tracks);

    _notesEditor.SetState(&state);
    _notesEditor.SetTracksManager(&_tracks);

    _inspectorWindow.SetState(&state);
    _inspectorWindow.SetTracksManager(&_tracks);
    _inspectorWindow.SetMidiIn(midiIn);
    _inspectorWindow.SetVstPluginLoader(_vstPluginService);

    _arpeggiatorPreviewService.SetState(&state);
    _arpeggiatorPreviewService.SetTracksManager(&_tracks);

    MainLoop();

    _tracks.CleanupInstruments();

    delete _vstPluginService;

    // Cleanup
    ImGui_ImplGlfwGL2_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();

    return 0;
}
