// ImGui - standalone example application for GLFW + OpenGL2, using legacy fixed pipeline
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)

// **DO NOT USE THIS CODE IF YOUR CODE/ENGINE IS USING MODERN OPENGL (SHADERS, VBO, VAO, etc.)**
// **Prefer using the code in the opengl3_example/ folder**
// See imgui_impl_glfw.cpp for details.

#include <glad/glad.h>

#include <GL/wglext.h>
#include <algorithm>
#include <commdlg.h>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <sstream>
#include <windows.h>

#include "imgui.h"
#include "imgui_impl_win32_gl2.h"
#include "imgui_internal.h"

#include "IconsFontaudio.h"
#include "IconsForkAwesome.h"
#include "RtError.h"
#include "RtMidi.h"
#include "vstplugin.h"
#include "wasapi.h"

namespace ImGui
{
    void MoveCursorPos(
        ImVec2 delta)
    {
        auto pos = GetCursorPos();
        SetCursorPos(ImVec2(pos.x + delta.x, pos.y + delta.y));
    }
} // namespace ImGui

class State
{
public:
    int _width;
    int _height;
};

class Instrument
{
public:
    std::string _name;
    int _midiChannel;
    VstPlugin *_plugin = nullptr;
};

enum class MidiEventTypes
{
    M_NOTE = 1,       // note
    M_CONTROLLER = 2, // controller
    M_PGMCHANGE = 3,  // program change
    M_PRESSURE = 4    // polyphonic aftertouch
};

struct MidiEvent
{
    MidiEvent();
    unsigned int channel; // the midi channel for the event
    MidiEventTypes type;  // type=1 for note, type=2 for controller
    unsigned int num;     // note, controller or program number
    unsigned int value;   // velocity or controller value
};

enum MidiControllers
{
    C_bankselectmsb = 0,
    C_pitchwheel = 1000,
    C_NULL = 1001,
    C_expression = 11,
    C_panning = 10,
    C_bankselectlsb = 32,
    C_filtercutoff = 74,
    C_filterq = 71,
    C_bandwidth = 75,
    C_modwheel = 1,
    C_fmamp = 76,
    C_volume = 7,
    C_sustain = 64,
    C_allnotesoff = 123,
    C_allsoundsoff = 120,
    C_resetallcontrollers = 121,
    C_portamento = 65,
    C_resonance_center = 77,
    C_resonance_bandwidth = 78,

    C_dataentryhi = 0x06,
    C_dataentrylo = 0x26,
    C_nrpnhi = 99,
    C_nrpnlo = 98
};

MidiEvent::MidiEvent()
    : channel(0),
      type(MidiEventTypes::M_NOTE),
      num(0),
      value(0)
{}

class Region
{
public:
    int _length;
    std::map<int, MidiEvent> _events; // the int key of the map is the relaive start of the note from the beginning of the region
};

class Track
{
public:
    Instrument *_instrument = nullptr;
    std::map<int, Region> _regions; // the int key of the map is the absolute start (in ms?) of the region from the beginning of the song
    std::string _name;
    bool _muted = false;
};

static RtMidiIn *midiIn = nullptr;
static State state;
static std::vector<Instrument *> instruments;
static std::vector<Track *> tracks;
static Track *activeTrack = nullptr;
static Track *soloTrack = nullptr;

// This function is called from Wasapi::threadFunc() which is running in audio thread.
bool refillCallback(
    std::vector<Track *> const &tracks,
    float *const data,
    uint32_t sampleCount,
    const WAVEFORMATEX *const mixFormat)
{
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
                    if (!track->_muted && (soloTrack == track || soloTrack == nullptr))
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

void SendMidiNote(
    int midiChannel,
    int noteNumber,
    bool onOff,
    int velocity)
{
    for (auto instrument : instruments)
    {
        instrument->_plugin->sendMidiNote(midiChannel, noteNumber, onOff, velocity);
    }
}

static std::map<int, bool> _keyStates;

void HandleKeyUpDown(int noteNumber)
{
    bool keyState = false;

    if (_keyStates.find(noteNumber) != _keyStates.end())
    {
        keyState = _keyStates[noteNumber];
    }

    if (!ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
    {
        if (keyState)
        {
            SendMidiNote(0, noteNumber, false, 0);
            _keyStates[noteNumber] = false;
        }

        return;
    }

    if (ImGui::IsMouseDown(0) && !keyState)
    {
        SendMidiNote(0, noteNumber, true, 100);
        _keyStates[noteNumber] = true;
    }
    else if (ImGui::IsMouseReleased(0) && keyState)
    {
        SendMidiNote(0, noteNumber, false, 0);
        _keyStates[noteNumber] = false;
    }
}

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

            n = firstKeyNoteNumber + (i * 12);
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

            SendMidiNote(ev.channel, ev.num, false, ev.value);
            break;
        }
        case 0x90: //Note On
        {
            ev.type = MidiEventTypes::M_NOTE;
            ev.channel = chan;
            ev.num = message->at(1);
            ev.value = message->at(2);

            SendMidiNote(ev.channel, ev.num, true, ev.value);
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
            ev.num = C_pitchwheel;
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

Track *addVstTrack(
    wchar_t const *plugin = nullptr)
{
    std::stringstream instrumentName;
    instrumentName << "Instrument " << (tracks.size() + 1);

    auto newi = new Instrument();
    newi->_name = instrumentName.str();
    newi->_midiChannel = 0;
    newi->_plugin = nullptr;
    if (plugin != nullptr)
    {
        newi->_plugin = new VstPlugin();
        newi->_plugin->init(plugin);
    }

    instruments.push_back(newi);

    std::stringstream trackName;
    trackName << "Track " << (tracks.size() + 1);

    auto newt = new Track();
    newt->_instrument = newi;
    newt->_name = trackName.str();
    tracks.push_back(newt);

    return newt;
}

void removeTrack(Track *track)
{
    if (track == nullptr)
    {
        return;
    }

    for (auto t = tracks.begin(); t != tracks.end(); ++t)
    {
        if (*t == track)
        {
            tracks.erase(t);
            delete track;
            break;
        }
    }
}

static float tracksScrolly = 0;

bool ActiveButton(char const *label, bool active)
{
    if (active)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
    }

    auto result = ImGui::Button(label);

    if (active)
    {
        ImGui::PopStyleColor();
    }

    return result;
}

int main(int, char **)
{
    if (!glfwInit())
    {
        return 1;
    }

    midiIn = new RtMidiIn();
    midiIn->setCallback(callback);

    GLFWwindow *window = glfwCreateWindow(1280, 720, L"ImGui OpenGL2 example", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup ImGui binding
    ImGui::CreateContext();

    ImGui_ImplGlfwGL2_Init(window, true);

    // Setup style
    ImGui::StyleColorsDark();
    ImGui::GetStyle().WindowRounding = 0;
    ImGui::GetStyle().ChildRounding = 0;

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
    io.Fonts->AddFontFromFileTTF("fonts/forkawesome-webfont.ttf", 12.0f, &config, forkawesome_icon_ranges);

    io.Fonts->Build();
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    activeTrack = addVstTrack(L"BBC Symphony Orchestra (64 Bit).dll");
    addVstTrack(L"LABS (64 Bit).dll");

    struct Key
    {
        explicit Key(int midiNote) : midiNote(midiNote), status(false) {}
        int midiNote;
        bool status;
    };

    std::map<int, Key> keyMap{
        {'Z', Key(60)}, // C-4
        {'S', Key(61)}, // C#4
        {'X', Key(62)}, // D-4
        {'D', Key(63)}, // D#4
        {'C', Key(64)}, // E-4
        {'V', Key(65)}, // F-4
        {'G', Key(66)}, // F#4
        {'B', Key(67)}, // G-4
        {'H', Key(68)}, // G#4
        {'N', Key(69)}, // A-4
        {'J', Key(70)}, // A#4
        {'M', Key(71)}, // B-4

        {'Q', Key(72)}, // C-5
        {'2', Key(73)}, // C#5
        {'W', Key(74)}, // D-5
        {'3', Key(75)}, // D#5
        {'E', Key(76)}, // E-5
        {'R', Key(77)}, // F-5
        {'5', Key(78)}, // F#5
        {'T', Key(79)}, // G-5
        {'6', Key(80)}, // G#5
        {'Y', Key(81)}, // A-5
        {'7', Key(82)}, // A#5
        {'U', Key(83)}, // B-5

        {'I', Key(84)}, // C-6
        {'9', Key(85)}, // C#6
        {'O', Key(86)}, // D-6
        {'0', Key(87)}, // D#6
        {'P', Key(88)}, // E-6
    };

    const int toolbarHeight = 80;
    const int pianoHeight = 180;
    const int inspectorWidth = 350;

    static int editTrackName = -1;
    static char editTrackBuffer[128] = {0};

    {
        Wasapi wasapi([&](float *const data, uint32_t availableFrameCount, const WAVEFORMATEX *const mixFormat) {
            return refillCallback(tracks, data, availableFrameCount, mixFormat);
        });

        // Main loop
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            ImGui_ImplGlfwGL2_NewFrame();
            glfwGetWindowSize(window, &(state._width), &(state._height));

            ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
            {
                ImGui::SetWindowPos(ImVec2(0, toolbarHeight));
                ImGui::SetWindowSize(ImVec2(inspectorWidth, state._height - toolbarHeight));

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
                if (ImGui::CollapsingHeader("Quick Help", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Text("Help!");
                }

                if (activeTrack != nullptr)
                {
                    if (ImGui::CollapsingHeader((std::string("Track: ") + activeTrack->_name).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        if (activeTrack->_instrument != nullptr)
                        {
                            auto vstPlugin = activeTrack->_instrument->_plugin;

                            if (vstPlugin == nullptr)
                            {
                                if (ImGui::Button("Add plugin"))
                                {
                                    activeTrack->_instrument->_plugin = loadPlugin();
                                    activeTrack->_instrument->_plugin->title = activeTrack->_instrument->_name.c_str();
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
                                    activeTrack->_instrument->_plugin = loadPlugin();
                                    activeTrack->_instrument->_plugin->title = activeTrack->_instrument->_name.c_str();
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
                        ImGui::Separator();
                    }
                }
            }
            ImGui::End();

            ImGui::Begin("Tracks", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
            {
                auto pos = ImVec2(inspectorWidth, toolbarHeight);
                ImGui::SetWindowPos(pos);
                auto size = ImVec2(state._width - pos.x, state._height - toolbarHeight - pianoHeight);
                ImGui::SetWindowSize(size);

                const int trackHeaderWidth = 200;
                static int trackHeight = 200;
                const int trackToolsHeight = 40;
                int fullWidth = 1500 + 200;
                int fullHeight = tracks.size() * (trackHeight + ImGui::GetStyle().ItemSpacing.y);

                ImGui::BeginChild(
                    "track_tools",
                    ImVec2(size.y, trackToolsHeight));
                {
                    ImGui::Button(ICON_FK_PLUS);
                    if (ImGui::BeginPopupContextItem("item context menu", 0))
                    {
                        if (ImGui::MenuItem("VST"))
                        {
                            activeTrack = addVstTrack();
                        }

                        ImGui::EndPopup();
                    }

                    ImGui::SameLine();

                    ImGui::PushItemWidth(100);
                    ImGui::SliderInt("track height", &trackHeight, 60, 300);
                    ImGui::PopItemWidth();
                }
                ImGui::EndChild();

                auto trackbgcol = ImColor(55, 55, 55, 55);
                auto trackaltbgcol = ImColor(0, 0, 0, 0);
                auto trackactivebgcol = ImColor(55, 88, 155, 55);
                auto contentTop = ImGui::GetCursorPosY();
                ImGui::MoveCursorPos(ImVec2(trackHeaderWidth, 0));
                ImGui::BeginChild(
                    "tracksContainer",
                    ImVec2(size.x - trackHeaderWidth - ImGui::GetStyle().ItemSpacing.x, -10),
                    false,
                    ImGuiWindowFlags_HorizontalScrollbar);
                {
                    auto drawList = ImGui::GetWindowDrawList();
                    ImGui::BeginGroup();
                    {
                        int t = 0;
                        for (auto track : tracks)
                        {
                            auto pp = ImGui::GetCursorScreenPos();
                            if (track == activeTrack)
                            {
                                drawList->AddRectFilled(pp, ImVec2(pp.x + fullWidth, pp.y + trackHeight), trackactivebgcol);
                            }
                            else if (t % 2 == 0)
                            {
                                drawList->AddRectFilled(pp, ImVec2(pp.x + fullWidth, pp.y + trackHeight), trackbgcol);
                            }
                            else
                            {
                                drawList->AddRectFilled(pp, ImVec2(pp.x + fullWidth, pp.y + trackHeight), trackaltbgcol);
                            }
                            ImGui::PushID(t);

                            if (ImGui::InvisibleButton(track->_name.c_str(), ImVec2(fullWidth, trackHeight)))
                            {
                                activeTrack = track;
                            }

                            ImGui::PopID();
                            t++;
                        }
                    }
                    ImGui::EndGroup();

                    tracksScrolly = ImGui::GetScrollY();
                }
                ImGui::EndChild();

                ImGui::SetCursorPosY(contentTop);
                ImGui::BeginChild(
                    "headersContainer",
                    ImVec2(trackHeaderWidth, -(10)),
                    false,
                    ImGuiWindowFlags_NoScrollbar);
                {
                    auto drawList = ImGui::GetWindowDrawList();
                    ImGui::SetScrollY(tracksScrolly);

                    ImGui::BeginChild(
                        "headers",
                        ImVec2(trackHeaderWidth, fullHeight + ImGui::GetStyle().ScrollbarSize),
                        false,
                        ImGuiWindowFlags_NoScrollbar);
                    {
                        int t = 0;
                        for (auto track : tracks)
                        {
                            auto pp = ImGui::GetCursorScreenPos();
                            if (track == activeTrack)
                            {
                                drawList->AddRectFilled(pp, ImVec2(pp.x + fullWidth, pp.y + trackHeight), trackactivebgcol);
                            }
                            else if (t % 2 == 0)
                            {
                                drawList->AddRectFilled(pp, ImVec2(pp.x + fullWidth, pp.y + trackHeight), trackbgcol);
                            }
                            else
                            {
                                drawList->AddRectFilled(pp, ImVec2(pp.x + fullWidth, pp.y + trackHeight), trackaltbgcol);
                            }

                            ImGui::PushID(t);
                            auto p = ImGui::GetCursorPos();
                            ImGui::BeginGroup();
                            {
                                if (ActiveButton(ICON_FAD_MUTE, track->_muted))
                                {
                                    track->_muted = !track->_muted;
                                    if (track->_muted && soloTrack == track)
                                    {
                                        soloTrack = nullptr;
                                    }
                                    activeTrack = track;
                                }

                                ImGui::SameLine();

                                if (ActiveButton(ICON_FAD_SOLO, soloTrack == track))
                                {
                                    if (soloTrack != track)
                                    {
                                        soloTrack = track;
                                        track->_muted = false;
                                    }
                                    else
                                    {
                                        soloTrack = nullptr;
                                    }
                                    activeTrack = track;
                                }

                                ImGui::SameLine();

                                if (editTrackName == t)
                                {
                                    ImGui::SetKeyboardFocusHere();
                                    if (ImGui::InputText("##editName", editTrackBuffer, 128, ImGuiInputTextFlags_EnterReturnsTrue))
                                    {
                                        track->_name = editTrackBuffer;
                                        editTrackName = -1;
                                    }
                                }
                                else
                                {
                                    ImGui::Text("%s", track->_name.c_str());
                                    if (ImGui::IsItemClicked())
                                    {
                                        editTrackName = t;
                                        strcpy_s(editTrackBuffer, 128, track->_name.c_str());
                                        activeTrack = track;
                                    }
                                }
                            }
                            ImGui::EndGroup();

                            if (ImGui::BeginPopupContextItem("track context menu"))
                            {
                                if (ImGui::MenuItem("Remove track"))
                                {
                                    removeTrack(track);
                                }

                                ImGui::EndPopup();
                            }

                            ImGui::SetCursorPos(ImVec2(p.x, p.y + trackHeight + ImGui::GetStyle().ItemSpacing.y));
                            ImGui::PopID();
                            t++;
                        }
                    }
                    ImGui::EndChild();
                }
                ImGui::EndChild();
            }
            ImGui::End();

            auto pos = ImVec2(inspectorWidth, state._height - pianoHeight);
            auto size = ImVec2(state._width - pos.x, pianoHeight);

            PianoWindow(pos, size, 8);

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

            if (instruments.size() > 0 && editTrackName < 0)
            {
                for (auto &e : keyMap)
                {
                    auto &key = e.second;
                    const auto on = (GetKeyState(e.first) & 0x8000) != 0;
                    if (key.status != on)
                    {
                        key.status = on;
                        SendMidiNote(0, key.midiNote, on, 100);
                    }
                }
            }
        }
    }

    while (!instruments.empty())
    {
        auto item = instruments.back();
        if (item->_plugin != nullptr)
        {
            delete item->_plugin;
        }
        instruments.pop_back();
        delete item;
    }

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
