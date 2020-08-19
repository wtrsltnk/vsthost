#include "inspectorwindow.h"

#include "instrument.h"

InspectorWindow::InspectorWindow()
{
}

void InspectorWindow::SetState(
    State *state)
{
    _state = state;
}

void InspectorWindow::SetTracksManager(
    ITracksManager *tracks)
{
    _tracks = tracks;
}

void InspectorWindow::SetMidiIn(
    RtMidiIn *midiIn)
{
    _midiIn = midiIn;
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
    if (GetOpenFileName(&ofn) != 0)
    {
        auto result = new VstPlugin();
        result->init(fn);
        return result;
    }

    return nullptr;
}

void InspectorWindow::Render(
    ImVec2 const &pos,
    ImVec2 const &size)
{
    ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
    {
        ImGui::SetWindowPos(pos);
        ImGui::SetWindowSize(size);

        if (ImGui::CollapsingHeader("Midi", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Midi ports");
            int ports = _midiIn->getPortCount();
            std::string portName;
            static int currentPort = -1;
            ImGui::BeginGroup();
            for (int i = 0; i < ports; i++)
            {
                try
                {
                    portName = _midiIn->getPortName(i);
                    if (ImGui::RadioButton(portName.c_str(), currentPort == i))
                    {
                        _midiIn->closePort();
                        _midiIn->openPort(i);
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

        if (_tracks->GetActiveTrack() != nullptr)
        {
            if (ImGui::CollapsingHeader((std::string("Track: ") + _tracks->GetActiveTrack()->GetName()).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::ColorEdit4(
                    "MyColor##track",
                    _tracks->GetActiveTrack()->GetColor(),
                    ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);

                if (_tracks->GetActiveTrack()->GetInstrument() != nullptr)
                {
                    auto vstPlugin = _tracks->GetActiveTrack()->GetInstrument()->_plugin;

                    if (vstPlugin == nullptr)
                    {
                        if (ImGui::Button("Add plugin"))
                        {
                            _tracks->GetActiveTrack()->GetInstrument()->_plugin = loadPlugin();
                            _tracks->GetActiveTrack()->GetInstrument()->_plugin->title = _tracks->GetActiveTrack()->GetInstrument()->_name.c_str();
                        }
                    }
                    else
                    {
                        ImGui::Text(
                            "%s by %s",
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
                            auto plugin = loadPlugin();
                            if (plugin != nullptr)
                            {
                                _tracks->GetActiveTrack()->GetInstrument()->_plugin = plugin;
                                _tracks->GetActiveTrack()->GetInstrument()->_plugin->title = _tracks->GetActiveTrack()->GetInstrument()->_name.c_str();
                            }
                        }
                        ImGui::SameLine();
                        if (!vstPlugin->isEditorOpen())
                        {
                            if (ImGui::Button("Open plugin"))
                            {
                                vstPlugin->openEditor(nullptr);
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
            }
        }

        auto track = std::get<ITrack *>(_tracks->GetActiveRegion());
        if (track == _tracks->GetActiveTrack())
        {
            auto regionStart = std::get<long>(_tracks->GetActiveRegion());
            if (track->Regions().find(regionStart) != track->Regions().end())
            {
                auto &region = track->GetRegion(regionStart);

                if (ImGui::CollapsingHeader((std::string("Region: ") + _tracks->GetActiveTrack()->GetName()).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Text("# of midi events: %llu", region._events.size());
                }
            }
        }

        if (ImGui::CollapsingHeader("Quick Help", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Help!");
        }
    }

    ImGui::End();
}
