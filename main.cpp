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

#include "imgui.h"
#include "imgui_impl_win32_gl2.h"

#include "wasapi.h"
#include "vstplugin.h"

class Instrument
{
public:
    std::string _name;
    int _midiChannel;
    VstPlugin *_plugin;
};

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

#include <windows.h>
#include <commdlg.h>

VstPlugin* addPlugin(HWND hwnd)
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

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        ImGui_ImplGlfwGL2_NewFrame();

        // 1. Show a simple window.
        // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets automatically appears in a window called "Debug".
        {
            static float f = 0.0f;
            ImGui::Text("Hello, world!");                           // Display some text (you can use a format string too)
            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f    
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our windows open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::Separator();
            for (Instrument &instrument : instruments)
            {
                ImGui::Text(instrument._name.c_str());
                auto vstPlugin = instrument._plugin;

                std::stringstream ss;
                if (vstPlugin == nullptr)
                {
                    ss << "Add plugin##" << &instrument;
                    if (ImGui::Button(ss.str().c_str()))
                    {
                        instrument._plugin = addPlugin(window->hwnd);
                    }
                    ImGui::Separator();
                    continue;
                }
                if (!vstPlugin->isEditorOpen())
                {
                    ss << "Open plugin##" << vstPlugin;
                    if (ImGui::Button(ss.str().c_str()))
                    {
                        vstPlugin->openEditor(window->hwnd);
                    }
                }
                else
                {
                    ss << "Close plugin##" << vstPlugin;
                    if (ImGui::Button(ss.str().c_str()))
                    {
                        vstPlugin->closeEditor();
                    }
                }
                ImGui::Separator();
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
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        }

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
            for(auto& e : keyMap) {
                auto& key = e.second;
                const auto on = (GetKeyState(e.first) & 0x8000) != 0;
                if(key.status != on) {
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
