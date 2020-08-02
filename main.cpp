// ImGui - standalone example application for GLFW + OpenGL2, using legacy fixed pipeline
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)

// **DO NOT USE THIS CODE IF YOUR CODE/ENGINE IS USING MODERN OPENGL (SHADERS, VBO, VAO, etc.)**
// **Prefer using the code in the opengl3_example/ folder**
// See imgui_impl_glfw.cpp for details.

#include <glad/glad.h>

#include <GL/wglext.h>
#include <algorithm>
#include <chrono>
#include <commdlg.h>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
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
#include "imguiutils.h"
#include "instrument.h"
#include "midicontrollers.h"
#include "midievent.h"
#include "region.h"
#include "state.h"
#include "track.h"
#include "trackseditor.h"
#include "tracksmanager.h"
#include "vstplugin.h"
#include "wasapi.h"

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
static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
static TracksEditor _tracksEditor;

void KillAllNotes()
{
    for (auto &instrument : _tracks.instruments)
    {
        for (int channel = 0; channel < 16; channel++)
        {
            for (int note = 0; note < 128; note++)
            {
                instrument->_plugin->sendMidiNote(
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
    std::vector<Track *> const &tracks,
    float *const data,
    uint32_t sampleCount,
    const WAVEFORMATEX *const mixFormat)
{
    auto start = state._cursor;
    auto diff = sampleCount * (1000.0 / mixFormat->nSamplesPerSec);
    state.UpdateByDiff(diff);
    auto end = state._cursor;

    for (auto track : tracks)
    {
        for (auto region : track->_regions)
        {
            if (region.first > end) continue;
            if (region.first + region.second._length < start) continue;

            for (auto event : region.second._events)
            {
                if ((event.first + region.first) > end) continue;
                if ((event.first + region.first) < start) continue;
                for (auto m : event.second)
                {
                    track->_instrument->_plugin->sendMidiNote(
                        m.channel,
                        m.num,
                        m.value != 0,
                        m.value);
                }
            }
        }
    }

    const auto nDstChannels = mixFormat->nChannels;

    for (size_t iFrame = 0; iFrame < sampleCount; ++iFrame)
    {
        for (size_t iChannel = 0; iChannel < nDstChannels; ++iChannel)
        {
            const unsigned int wasapiWriteIndex = iFrame * nDstChannels + iChannel;
            *(data + wasapiWriteIndex) = 0;
        }
    }

    for (auto track : tracks)
    {
        auto tmpsampleCount = sampleCount;
        auto vstPlugin = track->_instrument->_plugin;
        if (vstPlugin == nullptr)
        {
            continue;
        }
        vstPlugin->processEvents();

        const auto nSrcChannels = vstPlugin->getChannelCount();
        const auto vstSamplesPerBlock = vstPlugin->getBlockSize();

        int ofs = 0;
        while (tmpsampleCount > 0)
        {
            size_t outputFrameCount = 0;
            float **vstOutput = vstPlugin->processAudio(tmpsampleCount, outputFrameCount);

            if (vstOutput == nullptr)
            {
                break;
            }

            // VST vstOutput[][] format :
            //  vstOutput[a][b]
            //      channel = a % vstPlugin.getChannelCount()
            //      frame   = b + floor(a/2) * vstPlugin.getBlockSize()

            // wasapi data[] format :
            //  data[x]
            //      channel = x % mixFormat->nChannels
            //      frame   = floor(x / mixFormat->nChannels);

            const auto nFrame = outputFrameCount;
            for (size_t iFrame = 0; iFrame < nFrame; ++iFrame)
            {
                for (size_t iChannel = 0; iChannel < nDstChannels; ++iChannel)
                {
                    const unsigned int sChannel = iChannel % nSrcChannels;
                    const unsigned int vstOutputPage = (iFrame / vstSamplesPerBlock) * sChannel + sChannel;
                    const unsigned int vstOutputIndex = (iFrame % vstSamplesPerBlock);
                    const unsigned int wasapiWriteIndex = iFrame * nDstChannels + iChannel;
                    if (!track->_muted && (_tracks.soloTrack == track || _tracks.soloTrack == nullptr) && data != 0)
                    {
                        *(data + ofs + wasapiWriteIndex) += vstOutput[vstOutputPage][vstOutputIndex];
                    }
                }
            }

            tmpsampleCount -= nFrame;
            ofs += nFrame * nDstChannels;
        }
    }

    return true;
}

VstPlugin *loadPlugin()
{
    wchar_t fn[MAX_PATH + 1];
    OPENFILENAME ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = L"VSTi DLL(*.dll)\0*.dll\0All Files(*.*)\0*.*\0\0";
    ofn.lpstrFile = fn;
    ofn.nMaxFile = _countof(fn);
    ofn.lpstrTitle = L"Select VST DLL";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_ENABLESIZING;
    if (GetOpenFileName(&ofn))
    {
        auto result = new VstPlugin();
        result->init(fn);
        return result;
    }

    return nullptr;
}

void HandleIncomingMidiEvent(
    int midiChannel,
    int noteNumber,
    bool onOff,
    int velocity)
{
    if (state.IsRecording())
    {
        for (auto &track : _tracks.tracks)
        {
            if (!track->_readyForRecord)
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

    for (auto &instrument : _tracks.instruments)
    {
        instrument->_plugin->sendMidiNote(
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

const int Note_C_OffsetFromC = 0;
const int Note_CSharp_OffsetFromC = 1;
const int Note_D_OffsetFromC = 2;
const int Note_DSharp_OffsetFromC = 3;
const int Note_E_OffsetFromC = 4;
const int Note_F_OffsetFromC = 5;
const int Note_FSharp_OffsetFromC = 6;
const int Note_G_OffsetFromC = 7;
const int Note_GSharp_OffsetFromC = 8;
const int Note_A_OffsetFromC = 9;
const int Note_ASharp_OffsetFromC = 10;
const int Note_B_OffsetFromC = 11;
const int firstKeyNoteNumber = 24;

char const *NoteToString(
    unsigned int note);

void PianoWindow(
    ImVec2 const &pos,
    ImVec2 const &size,
    int octaves)
{
    ImGui::Begin("Piano", nullptr, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
    {
        ImGui::SetWindowPos(pos);
        ImGui::SetWindowSize(size);

        const int keyWidth = 32;
        const int keyHeight = 64;

        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(50, 50, 50));

        auto drawPos = ImGui::GetCursorScreenPos();
        auto drawHeight = ImGui::GetContentRegionAvail().y;
        int octaveWidth = (keyWidth + ImGui::GetStyle().ItemSpacing.x) * 7;
        int n;
        for (int i = 0; i < octaves; i++)
        {
            auto a = ImVec2(drawPos.x - (ImGui::GetStyle().ItemSpacing.x / 2) + octaveWidth * i, drawPos.y);
            auto b = ImVec2(a.x + octaveWidth, a.y + drawHeight);
            ImGui::GetWindowDrawList()->AddRectFilled(a, b, IM_COL32(66, 150, 249, i * 20));

            ImGui::PushID(i);
            if (i == 0)
            {
                ImGui::InvisibleButton("HalfSpace", ImVec2((keyWidth / 2) - (ImGui::GetStyle().ItemSpacing.x / 2), keyHeight));
            }

            ImGui::SameLine();

            n = firstKeyNoteNumber + (i * 12) + Note_CSharp_OffsetFromC;
            ImGui::Button(NoteToString(n), ImVec2(keyWidth, keyHeight));
            HandleKeyUpDown(n);
            ImGui::SameLine();

            n = firstKeyNoteNumber + (i * 12) + Note_DSharp_OffsetFromC;
            ImGui::Button(NoteToString(n), ImVec2(keyWidth, keyHeight));
            HandleKeyUpDown(n);
            ImGui::SameLine();

            ImGui::InvisibleButton("SpaceE", ImVec2(keyWidth, keyHeight));
            ImGui::SameLine();

            n = firstKeyNoteNumber + (i * 12) + Note_FSharp_OffsetFromC;
            ImGui::Button(NoteToString(n), ImVec2(keyWidth, keyHeight));
            HandleKeyUpDown(n);
            ImGui::SameLine();

            n = firstKeyNoteNumber + (i * 12) + Note_GSharp_OffsetFromC;
            ImGui::Button(NoteToString(n), ImVec2(keyWidth, keyHeight));
            HandleKeyUpDown(n);
            ImGui::SameLine();

            n = firstKeyNoteNumber + (i * 12) + Note_ASharp_OffsetFromC;
            ImGui::Button(NoteToString(n), ImVec2(keyWidth, keyHeight));
            HandleKeyUpDown(n);
            ImGui::SameLine();

            ImGui::InvisibleButton("HalfSpace", ImVec2(keyWidth, keyHeight));

            ImGui::PopID();
        }

        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor(255, 255, 255));
        ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor(20, 20, 20));

        for (int i = 0; i < octaves; i++)
        {
            ImGui::PushID(i);

            if (i > 0)
            {
                ImGui::SameLine();
            }

            n = firstKeyNoteNumber + (i * 12) + Note_C_OffsetFromC;
            ImGui::Button(NoteToString(n), ImVec2(keyWidth, keyHeight));
            HandleKeyUpDown(n);
            ImGui::SameLine();

            n = firstKeyNoteNumber + (i * 12) + Note_D_OffsetFromC;
            ImGui::Button(NoteToString(n), ImVec2(keyWidth, keyHeight));
            HandleKeyUpDown(n);
            ImGui::SameLine();

            n = firstKeyNoteNumber + (i * 12) + Note_E_OffsetFromC;
            ImGui::Button(NoteToString(n), ImVec2(keyWidth, keyHeight));
            HandleKeyUpDown(n);
            ImGui::SameLine();

            n = firstKeyNoteNumber + (i * 12) + Note_F_OffsetFromC;
            ImGui::Button(NoteToString(n), ImVec2(keyWidth, keyHeight));
            HandleKeyUpDown(n);
            ImGui::SameLine();

            n = firstKeyNoteNumber + (i * 12) + Note_G_OffsetFromC;
            ImGui::Button(NoteToString(n), ImVec2(keyWidth, keyHeight));
            HandleKeyUpDown(n);
            ImGui::SameLine();

            n = firstKeyNoteNumber + (i * 12) + Note_A_OffsetFromC;
            ImGui::Button(NoteToString(n), ImVec2(keyWidth, keyHeight));
            HandleKeyUpDown(n);
            ImGui::SameLine();

            n = firstKeyNoteNumber + (i * 12) + Note_B_OffsetFromC;
            ImGui::Button(NoteToString(n), ImVec2(keyWidth, keyHeight));
            HandleKeyUpDown(n);

            ImGui::PopID();
        }

        ImGui::PopStyleColor(2);
    }
    ImGui::End();
}

void callback(double /*timeStamp*/, std::vector<unsigned char> *message, void * /*userData*/)
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
            for (auto track : _tracks.tracks)
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

        ImGui::PushItemWidth(100);
        ImGui::SliderInt("bpm", (int *)&(state._bpm), 10, 200);
        ImGui::PopItemWidth();
    }
    ImGui::End();
}

const int toolbarHeight = 80;
const int pianoHeight = 180;
const int inspectorWidth = 350;

void InspectorWindow(
    int *currentInspectorWidth,
    ImVec2 const &pos,
    ImVec2 const &size)
{

    ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
    {
        ImGui::SetWindowPos(pos);
        ImGui::SetWindowSize(size);

        if (*currentInspectorWidth != inspectorWidth)
        {
            ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
            {
                ImGui::SetWindowPos(ImVec2(0, toolbarHeight));
                ImGui::SetWindowSize(ImVec2(*currentInspectorWidth, state._height - toolbarHeight));

                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
                if (ImGui::Button(ICON_FK_EYE))
                {
                    *currentInspectorWidth = inspectorWidth;
                }
                ImGui::PopStyleVar();
            }
            ImGui::End();
        }
        else
        {
            if (ImGui::Button(ICON_FK_EYE_SLASH))
            {
                *currentInspectorWidth = 36;
            }

            if (ImGui::CollapsingHeader("Midi", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Midi ports");
                int ports = midiIn->getPortCount();
                std::string portName;
                static int currentPort = -1;
                ImGui::BeginGroup();
                for (int i = 0; i < ports; i++)
                {
                    try
                    {
                        portName = midiIn->getPortName(i);
                        if (ImGui::RadioButton(portName.c_str(), currentPort == i))
                        {
                            midiIn->closePort();
                            midiIn->openPort(i);
                            currentPort = i;
                        }
                    }
                    catch (RtError &error)
                    {
                        error.printMessage();
                    }
                }
                ImGui::EndGroup();
            }

            if (_tracks.activeTrack != nullptr)
            {
                if (ImGui::CollapsingHeader((std::string("Track: ") + _tracks.activeTrack->_name).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                {
                    if (_tracks.activeTrack->_instrument != nullptr)
                    {
                        auto vstPlugin = _tracks.activeTrack->_instrument->_plugin;

                        if (vstPlugin == nullptr)
                        {
                            if (ImGui::Button("Add plugin"))
                            {
                                _tracks.activeTrack->_instrument->_plugin = loadPlugin();
                                _tracks.activeTrack->_instrument->_plugin->title = _tracks.activeTrack->_instrument->_name.c_str();
                            }
                            ImGui::Separator();
                        }
                        else
                        {
                            ImGui::Text("%s by %s",
                                        vstPlugin->getEffectName().c_str(),
                                        vstPlugin->getVendorName().c_str());

                            /* Save plugin data
void *getLen;
    int length = plugin->dispatcher(plugin,effGetChunk,0,0,&getLen,0.f);
*/

                            /* Load plugin data
plugin->dispatcher(plugin,effSetChunk,0,(VstInt32)tempLength,&buffer,0);
*/

                            if (ImGui::Button("Change plugin"))
                            {
                                _tracks.activeTrack->_instrument->_plugin = loadPlugin();
                                _tracks.activeTrack->_instrument->_plugin->title = _tracks.activeTrack->_instrument->_name.c_str();
                            }
                            ImGui::SameLine();
                            if (!vstPlugin->isEditorOpen())
                            {
                                if (ImGui::Button("Open plugin"))
                                {
                                    vstPlugin->openEditor(window->hwnd);
                                }
                            }
                            else
                            {
                                if (ImGui::Button("Close plugin"))
                                {
                                    vstPlugin->closeEditor();
                                }
                            }
                        }
                    }

                    //                    ImGui::ShowDemoWindow();
                    ImGui::ColorEdit4(
                        "MyColor##3",
                        (float *)&(_tracks.activeTrack->_color),
                        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);

                    ImGui::Separator();
                }
            }

            if (ImGui::CollapsingHeader("Quick Help", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Help!");
            }
        }
    }
    ImGui::End();
}

void MainMenu()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
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
        ImGui::EndMainMenuBar();
    }
}

void HandleKeyboardToMidiEvents()
{
    if (_tracks.instruments.size() > 0 && _tracksEditor.EditTrackName() < 0)
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
    RtMidiIn localMidiIn;
    midiIn = &localMidiIn;
    midiIn->setCallback(callback);

    Wasapi wasapi([&](float *const data, uint32_t availableFrameCount, const WAVEFORMATEX *const mixFormat) {
        return refillCallback(_tracks.tracks, data, availableFrameCount, mixFormat);
    });

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        if (glfwGetWindowAttrib(window, GLFW_FOCUSED))
        {
            HandleKeyboardToMidiEvents();
        }

        ImGui_ImplGlfwGL2_NewFrame();

        glfwGetWindowSize(window, &(state._width), &(state._height));

        static int currentInspectorWidth = inspectorWidth;

        MainMenu();

        ToolbarWindow(
            ImVec2(0, ImGui::GetTextLineHeightWithSpacing()),
            ImVec2(state._width, toolbarHeight - ImGui::GetTextLineHeightWithSpacing()));

        InspectorWindow(
            &currentInspectorWidth,
            ImVec2(0, toolbarHeight),
            ImVec2(currentInspectorWidth, state._height - toolbarHeight));

        _tracksEditor.Render(
            ImVec2(currentInspectorWidth, toolbarHeight),
            ImVec2(state._width - currentInspectorWidth, state._height - toolbarHeight - pianoHeight));

        PianoWindow(
            ImVec2(currentInspectorWidth, state._height - pianoHeight),
            ImVec2(state._width - currentInspectorWidth, pianoHeight),
            8);

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

int main(int, char **)
{
    if (!glfwInit())
    {
        return 1;
    }

    window = glfwCreateWindow(1280, 720, L"VstHost", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup ImGui binding
    ImGui::CreateContext();

    ImGui_ImplGlfwGL2_Init(window, true);

    // Setup style
    ImGui::StyleColorsDark();
    ImGui::GetStyle().WindowRounding = 0;
    ImGui::GetStyle().ChildRounding = 0;

    SetupFonts();

    _tracks.activeTrack = _tracks.AddVstTrack(L"BBC Symphony Orchestra (64 Bit).dll");

    _tracksEditor.SetState(&state);
    _tracksEditor.SetTracksManager(&_tracks);

    MainLoop();

    _tracks.CleanupInstruments();

    // Cleanup
    ImGui_ImplGlfwGL2_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();

    return 0;
}

char const *NoteToString(
    unsigned int note)
{
    switch (note)
    {
        case 127:
            return "G-9";
        case 126:
            return "F#9";
        case 125:
            return "F-9";
        case 124:
            return "E-9";
        case 123:
            return "D#9";
        case 122:
            return "D-9";
        case 121:
            return "C#9";
        case 120:
            return "C-9";
        case 119:
            return "B-8";
        case 118:
            return "A#8";
        case 117:
            return "A-8";
        case 116:
            return "G#8";
        case 115:
            return "G-8";
        case 114:
            return "F#8";
        case 113:
            return "F-8";
        case 112:
            return "E-8";
        case 111:
            return "D#8";
        case 110:
            return "D-8";
        case 109:
            return "C#8";
        case 108:
            return "C-8";
        case 107:
            return "B-7";
        case 106:
            return "A#7";
        case 105:
            return "A-7";
        case 104:
            return "G#7";
        case 103:
            return "G-7";
        case 102:
            return "F#7";
        case 101:
            return "F-7";
        case 100:
            return "E-7";
        case 99:
            return "D#7";
        case 98:
            return "D-7";
        case 97:
            return "C#7";
        case 96:
            return "C-7";
        case 95:
            return "B-6";
        case 94:
            return "A#6";
        case 93:
            return "A-6";
        case 92:
            return "G#6";
        case 91:
            return "G-6";
        case 90:
            return "F#6";
        case 89:
            return "F-6";
        case 88:
            return "E-6";
        case 87:
            return "D#6";
        case 86:
            return "D-6";
        case 85:
            return "C#6";
        case 84:
            return "C-6";
        case 83:
            return "B-5";
        case 82:
            return "A#5";
        case 81:
            return "A-5";
        case 80:
            return "G#5";
        case 79:
            return "G-5";
        case 78:
            return "F#5";
        case 77:
            return "F-5";
        case 76:
            return "E-5";
        case 75:
            return "D#5";
        case 74:
            return "D-5";
        case 73:
            return "C#5";
        case 72:
            return "C-5";
        case 71:
            return "B-4";
        case 70:
            return "A#4";
        case 69:
            return "A-4";
        case 68:
            return "G#4";
        case 67:
            return "G-4";
        case 66:
            return "F#4";
        case 65:
            return "F-4";
        case 64:
            return "E-4";
        case 63:
            return "D#4";
        case 62:
            return "D-4";
        case 61:
            return "C#4";
        case 60:
            return "C-4";
        case 59:
            return "B-3";
        case 58:
            return "A#3";
        case 57:
            return "A-3";
        case 56:
            return "G#3";
        case 55:
            return "G-3";
        case 54:
            return "F#3";
        case 53:
            return "F-3";
        case 52:
            return "E-3";
        case 51:
            return "D#3";
        case 50:
            return "D-3";
        case 49:
            return "C#3";
        case 48:
            return "C-3";
        case 47:
            return "B-2";
        case 46:
            return "A#2";
        case 45:
            return "A-2";
        case 44:
            return "G#2";
        case 43:
            return "G-2";
        case 42:
            return "F#2";
        case 41:
            return "F-2";
        case 40:
            return "E-2";
        case 39:
            return "D#2";
        case 38:
            return "D-2";
        case 37:
            return "C#2";
        case 36:
            return "C-2";
        case 35:
            return "B-1";
        case 34:
            return "A#1";
        case 33:
            return "A-1";
        case 32:
            return "G#1";
        case 31:
            return "G-1";
        case 30:
            return "F#1";
        case 29:
            return "F-1";
        case 28:
            return "E-1";
        case 27:
            return "D#1";
        case 26:
            return "D-1";
        case 25:
            return "C#1";
        case 24:
            return "C-1";
        case 23:
            return "B-0";
        case 22:
            return "A#0";
        case 21:
            return "A-0";
    }
    return "---";
}
