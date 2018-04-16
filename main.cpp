// ImGui - standalone example application for GLFW + OpenGL2, using legacy fixed pipeline
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)

// **DO NOT USE THIS CODE IF YOUR CODE/ENGINE IS USING MODERN OPENGL (SHADERS, VBO, VAO, etc.)**
// **Prefer using the code in the opengl3_example/ folder**
// See imgui_impl_glfw.cpp for details.

#include <glad/glad.h>
#include <GL/wglext.h>
#include <sstream>
#include <map>
#include <iostream>
#include <algorithm>

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_win32_gl2.h"

#include "wasapi.h"
#include "vstplugin.h"

class Settings
{
public:
    int _additionlTrackLength = 300;
};

class Instrument
{
public:
    std::string _name;
    int _midiChannel;
    VstPlugin *_plugin;
};

enum class RegionTypes
{
    Audio,
    Midi,
};

class Region
{
private:
    RegionTypes _type;
protected:
    Region(std::string const &name, RegionTypes type, int start);
public:
    virtual ~Region();

    std::string _name;
    int _start;

    RegionTypes Type() const;
    virtual int  Size() const = 0;
};

Region::Region(std::string const &name, RegionTypes type, int start)
    : _name(name), _type(type), _start(start)
{ }

Region::~Region()
{ }

RegionTypes Region::Type() const
{
    return _type;
}

#define MIN_REGION_SIZE 100

class MidiRegion : public Region
{
public:
    MidiRegion(std::string const &name, int start);
    virtual ~MidiRegion();

    virtual int Size() const;
};

MidiRegion::MidiRegion(std::string const &name, int start)
    : Region(name, RegionTypes::Midi, start)
{ }

MidiRegion::~MidiRegion()
{ }

int MidiRegion::Size() const
{
    return MIN_REGION_SIZE;
}

class AudioRegion : public Region
{
public:
    AudioRegion(std::string const &name, int start);
    virtual ~AudioRegion();

    virtual int Size() const;
};

AudioRegion::AudioRegion(std::string const &name, int start)
    : Region(name, RegionTypes::Audio, start)
{ }

AudioRegion::~AudioRegion()
{ }

int AudioRegion::Size() const
{
    return MIN_REGION_SIZE;
}

class Channel
{
public:
    virtual ~Channel();

    std::string _name;
};

Channel::~Channel() { }

class InputChannel : public Channel
{
public:
    virtual ~InputChannel();
};

InputChannel::~InputChannel() { }

class InstrumentChannel : public InputChannel
{
public:
    virtual ~InstrumentChannel();

    int _midiChannel;
};

InstrumentChannel::~InstrumentChannel() { }

class OutputChannel : public Channel
{
public:
    virtual ~OutputChannel();
};

OutputChannel::~OutputChannel() { }

class Track
{
public:
    InputChannel *_channel = nullptr;
    OutputChannel *_outputChannel = nullptr;
    std::string _name;
    std::vector<Region*> _regions;

    int TotalLength() const;
};

int Track::TotalLength() const
{
    //  This assumes the regions are ordered
    return _regions.back()->_start + _regions.back()->Size();
}

int LongestTrack(std::vector<Track> const &tracks)
{
    int result = 0;
    for (auto track : tracks)
    {
        if (result < track.TotalLength())
        {
            result = track.TotalLength();
        }
    }
    return result;
}

// This function is called from Wasapi::threadFunc() which is running in audio thread.
bool refillCallback(std::vector<Instrument>& instruments, float* const data, uint32_t availableFrameCount, const WAVEFORMATEX* const mixFormat) {

    const auto nDstChannels = mixFormat->nChannels;

    for (auto instrument : instruments)
    {
        auto vstPlugin = instrument._plugin;
        if (vstPlugin == nullptr)
        {
            continue;
        }
        vstPlugin->processEvents();

        const auto nSrcChannels = vstPlugin->getChannelCount();
        const auto vstSamplesPerBlock = vstPlugin->getBlockSize();

        int ofs = 0;
        while(availableFrameCount > 0) {
            size_t outputFrameCount = 0;
            float** vstOutput = vstPlugin->processAudio(availableFrameCount, outputFrameCount);

            // VST vstOutput[][] format :
            //  vstOutput[a][b]
            //      channel = a % vstPlugin.getChannelCount()
            //      frame   = b + floor(a/2) * vstPlugin.getBlockSize()

            // wasapi data[] format :
            //  data[x]
            //      channel = x % mixFormat->nChannels
            //      frame   = floor(x / mixFormat->nChannels);

            const auto nFrame = outputFrameCount;
            for(size_t iFrame = 0; iFrame < nFrame; ++iFrame) {
                for(size_t iChannel = 0; iChannel < nDstChannels; ++iChannel) {
                    const int sChannel = iChannel % nSrcChannels;
                    const int vstOutputPage = (iFrame / vstSamplesPerBlock) * sChannel + sChannel;
                    const int vstOutputIndex = (iFrame % vstSamplesPerBlock);
                    const int wasapiWriteIndex = iFrame * nDstChannels + iChannel;
                    *(data + ofs + wasapiWriteIndex) = vstOutput[vstOutputPage][vstOutputIndex];
                }
            }

            availableFrameCount -= nFrame;
            ofs += nFrame * nDstChannels;
        }
    }

    return true;
}

static int trackStartX = 0;
namespace ImGui
{
bool MovableRegion(Region *region, int &location)
{
    ImGui::SameLine();
    ImGuiContext* context = ImGui::GetCurrentContext();
    auto window = context->CurrentWindow;
    const ImGuiID id = window->GetID(region); // use the pointer address as identifier
    const ImVec2 text_pos(trackStartX, window->DC.CursorPos.y);

    ImGuiButtonFlags flags = 0;

    const ImGuiStyle& style = context->Style;

    ImVec2 handle_pos(text_pos.x + (location - (location % 20)), text_pos.y);

    // Rect

    ImRect rect;
    rect = ImRect(handle_pos.x,
                  handle_pos.y,
                  handle_pos.x + region->Size(),
                  handle_pos.y + 80);

    ItemSize(rect, style.FramePadding.y);
    if (!ItemAdd(rect, id))
    {
        return false;
    }

    bool hovered, held;
    bool pressed = ButtonBehavior(rect, id, &hovered, &held, flags);
    bool is_active = ImGui::IsItemActive();

    const ImGuiCol_ color_enum = (hovered
                                  || is_active
                                  || pressed
                                  || held) ? ImGuiCol_ButtonHovered : ImGuiCol_Button;

    ImGuiCol handle_color = ImColor(ImGui::GetStyle().Colors[color_enum]);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    draw_list->AddRectFilled(rect.Min, rect.Max, handle_color, 4.0f);
    draw_list->AddText(handle_pos, ImColor(ImGui::GetStyle().Colors[ImGuiCol_Text]), region->_name.c_str());

    if (is_active && ImGui::IsMouseDragging(0))
    {
        location += ImGui::GetIO().MouseDelta.x;

        if (location < 0)
        {
            location = 0;
        }
    }

    return is_active;
}
}


#include <windows.h>
#include <commdlg.h>

VstPlugin* loadPlugin(HWND hwnd)
{
    wchar_t fn[MAX_PATH+1] {};
    OPENFILENAME ofn { sizeof(ofn) };
    ofn.lpstrFilter = L"VSTi DLL(*.dll)\0*.dll\0All Files(*.*)\0*.*\0\0";
    ofn.lpstrFile   = fn;
    ofn.nMaxFile    = _countof(fn);
    ofn.lpstrTitle  = L"Select VST DLL";
    ofn.Flags       = OFN_FILEMUSTEXIST | OFN_ENABLESIZING;
    if (GetOpenFileName(&ofn))
    {
        return new VstPlugin(fn, hwnd);
    }

    return nullptr;
}

int main(int, char**)
{
    if (!glfwInit())
    {
        return 1;
    }

    GLFWwindow* window = glfwCreateWindow(1280, 720, L"ImGui OpenGL2 example", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup ImGui binding
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    ImGui_ImplGlfwGL2_Init(window, true);

    // Setup style
    ImGui::StyleColorsDark();

    ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\Tahoma.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());

    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    std::vector<Instrument> instruments;
    std::vector<Track> tracks;
    Settings settings;
    Region *activeRegion = nullptr;
    Track *activeTrack = nullptr;

    Track t;
    t._name = "Track 1";
    t._regions.push_back(new AudioRegion("Classic piano", 0));
    t._regions.push_back(new AudioRegion("Recorded drums", 200));
    tracks.push_back(t);

    Track t2;
    t2._name = "Track 2";
    t2._regions.push_back(new AudioRegion("Screeky guitar 2", 50));
    t2._regions.push_back(new AudioRegion("Screeky guitar", 170));
    tracks.push_back(t2);

    Wasapi wasapi { [&instruments](float* const data, uint32_t availableFrameCount, const WAVEFORMATEX* const mixFormat) {
            return refillCallback(instruments, data, availableFrameCount, mixFormat);
        }};

    struct Key {
        Key(int midiNote) : midiNote { midiNote } {}
        int     midiNote {};
        bool    status { false };
    };

    std::map<int, Key> keyMap {
        {'2', {61}}, {'3', {63}},              {'5', {66}}, {'6', {68}}, {'7', {70}},
        {'Q', {60}}, {'W', {62}}, {'E', {64}}, {'R', {65}}, {'T', {67}}, {'Y', {69}}, {'U', {71}}, {'I', {72}},

        {'S', {49}}, {'D', {51}},              {'G', {54}}, {'H', {56}}, {'J', {58}},
        {'Z', {48}}, {'X', {50}}, {'C', {52}}, {'V', {53}}, {'B', {55}}, {'N', {57}}, {'M', {59}}, {VK_OEM_COMMA, {60}},
    };
    char const *channelLabels[] {
        "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15"
    };

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplGlfwGL2_NewFrame();

        // 1. Show a simple window.
        ImGui::Begin("Instruments");
        {
            for (Instrument &instrument : instruments)
            {
                ImGui::PushID(int(&instrument));
                if (ImGui::CollapsingHeader(instrument._name.c_str()))
                {
                    ImGui::Combo("Channel", &(instrument._midiChannel), channelLabels, 16);
                    auto vstPlugin = instrument._plugin;

                    if (vstPlugin == nullptr)
                    {
                        if (ImGui::Button("Add plugin"))
                        {
                            instrument._plugin = loadPlugin(window->hwnd);
                        }
                        ImGui::Separator();
                    }
                    else
                    {
                        ImGui::Text(vstPlugin->getEffectName().c_str());
                        ImGui::SameLine();
                        ImGui::Text(" by ");
                        ImGui::SameLine();
                        ImGui::Text(vstPlugin->getVendorName().c_str());
                        if (vstPlugin != nullptr)
                        {
                            if (ImGui::Button("Change plugin"))
                            {
                                instrument._plugin = loadPlugin(window->hwnd);
                            }
                            ImGui::SameLine();
                        }
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
                ImGui::PopID();
            }

            if (ImGui::Button("Add instrument"))
            {
                static int counter = 1;
                std::stringstream ss;
                ss << "Instrument " << counter++;
                auto instrument = Instrument();
                instrument._name = ss.str();
                instrument._midiChannel = 0;
                instrument._plugin = nullptr;
                instruments.push_back(instrument);
            }
            ImGui::End();
        }

        ImGui::Begin("Tracks",nullptr, ImGuiWindowFlags_HorizontalScrollbar);
        {
            auto minimalWidth = LongestTrack(tracks) + settings._additionlTrackLength;
            ImGui::BeginChild("scrolling", ImVec2(std::max(float(minimalWidth), ImGui::GetWindowContentRegionWidth()), 0), true, 0);

            for (int i = 0; i < tracks.size(); i++)
            {
                ImGui::PushID(i);

                ImGuiContext* context = ImGui::GetCurrentContext();
                auto window = context->CurrentWindow;
                trackStartX = window->DC.CursorPos.x + 150;

                if (ImGui::Button(tracks[i]._name.c_str()))
                {
                    activeTrack = &tracks[i];
                    activeRegion = nullptr;
                }

                for (auto region : tracks[i]._regions)
                {
                    if (ImGui::MovableRegion(region, region->_start))
                    {
                        activeRegion = region;
                        activeTrack = &tracks[i];
                    }
                }

                ImGui::PopID();
                trackStartX = 0;
            }
            ImGui::EndChild();
        }
        ImGui::End();

        ImGui::Begin("Inspector");
        {
            if (ImGui::CollapsingHeader("Quick Help", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Help!");
            }

            if (activeRegion != nullptr)
            {
                if (ImGui::CollapsingHeader((std::string("Region: ") + activeRegion->_name).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Text("active region");
                }
            }

            if (activeTrack != nullptr)
            {
                if (ImGui::CollapsingHeader((std::string("Track: ") + activeTrack->_name).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                {
                    auto instrumentChannel = static_cast<InstrumentChannel*>(activeTrack->_channel);
                    if (instrumentChannel != nullptr)
                    {
                        ImGui::Combo("MIDI Channel", &(instrumentChannel->_midiChannel), channelLabels, 16);
                    }
                }
            }

            ImGui::BeginChild("channelstrip");
            ImGui::Text("This is the channelstrip");
            ImGui::Text("This is the channelstrip");
            ImGui::Text("This is the channelstrip");
            ImGui::Text("This is the channelstrip");
            ImGui::Text("This is the channelstrip");
            ImGui::Text("This is the channelstrip");
            ImGui::Text("This is the channelstrip");
            ImGui::Text("This is the channelstrip");
            ImGui::Text("This is the channelstrip");
            ImGui::Text("This is the channelstrip");
            ImGui::Text("This is the channelstrip");
            ImGui::Text("This is the channelstrip");
            ImGui::Text("This is the channelstrip");
            ImGui::End();
        }
        ImGui::End();

//        // 3. Show the ImGui demo window. Most of the sample code is in ImGui::ShowDemoWindow(). Read its code to learn more about Dear ImGui!
//        if (show_demo_window)
//        {
//            ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver); // Normally user code doesn't need/want to call this because positions are saved in .ini file anyway. Here we just want to make the demo initial state a bit more friendly!
//            ImGui::ShowDemoWindow(&show_demo_window);
//        }

        // Rendering
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Render();
        ImGui_ImplGlfwGL2_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);

        if (instruments.size() > 0)
        {
            for(auto& e : keyMap)
            {
                auto& key = e.second;
                const auto on = (GetKeyState(e.first) & 0x8000) != 0;
                if(key.status != on)
                {
                    key.status = on;
                    for (auto instrument : instruments)
                    {
                        if (instrument._plugin != nullptr)
                        {
                            instrument._plugin->sendMidiNote(0, key.midiNote, on, 100);
                        }
                    }
                }
            }
        }
    }

    while (!instruments.empty())
    {
        auto item = instruments.back();
        if (item._plugin != nullptr)
        {
            delete item._plugin;
        }
        instruments.pop_back();
    }

    // Cleanup
    ImGui_ImplGlfwGL2_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();

    return 0;
}
