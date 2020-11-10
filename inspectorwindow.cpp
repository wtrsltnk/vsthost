#include "inspectorwindow.h"

#include "IconsFontaudio.h"
#include "historymanager.h"
#include "instrument.h"
#include "ivstpluginservice.h"

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

void InspectorWindow::SetAudioOut(
    struct Wasapi *wasapi)
{
    _wasapi = wasapi;
}

void InspectorWindow::SetVstPluginLoader(
    IVstPluginService *loader)
{
    _vstPluginLoader = loader;
}

void InspectorWindow::Render(
    ImVec2 const &pos,
    ImVec2 const &size)
{
    ImGui::Begin("Inspector", nullptr, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
    {
        ImGui::SetWindowPos(pos);
        ImGui::SetWindowSize(size);

        bool midiPortsAvailable = _midiIn != nullptr && _midiIn->getPortCount() > 0;

        if (ImGui::CollapsingHeader("Midi in", midiPortsAvailable ? ImGuiTreeNodeFlags_DefaultOpen : 0))
        {
            ImGui::Text("Midi ports");
            int ports = _midiIn->getPortCount();
            std::string portName;
            static int currentPort = midiPortsAvailable ? 0 : -1;
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

        if (_wasapi != nullptr && ImGui::CollapsingHeader("Audio out"))
        {
            const std::string activeDevice(_wasapi->CurrentDevice().begin(), _wasapi->CurrentDevice().end());
            ImGui::Text("Active Audio Device:");
            ImGui::BulletText("%s", activeDevice.c_str());

            ImGui::Text("Other Audio Devices");
            ImGui::BeginGroup();
            for (auto &d : _wasapi->Devices())
            {
                if (d == _wasapi->CurrentDevice())
                {
                    continue;
                }

                const std::string s(d.begin(), d.end());

                ImGui::BulletText("%s", s.c_str());
            }
            ImGui::EndGroup();
        }

        if (_tracks->GetActiveTrackId() != Track::Null)
        {
            auto &track = _tracks->GetTrack(_tracks->GetActiveTrackId());

            if (ImGui::CollapsingHeader((std::string("Track: ") + track.GetName()).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (track.GetInstrument() != nullptr)
                {
                    auto vstPlugin = track.GetInstrument()->Plugin();

                    if (vstPlugin == nullptr)
                    {
                        ImGui::ColorEdit4(
                            "MyColor##track",
                            track.GetColor(),
                            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);

                        ImGui::SameLine();

                        if (_vstPluginLoader != nullptr && ImGui::Button("Add plugin"))
                        {
                            track.GetInstrument()->SetPlugin(_vstPluginLoader->LoadFromFileDialog());
                        }
                    }
                    else
                    {
                        /* Save plugin data
void *getLen;
    int length = plugin->dispatcher(plugin,effGetChunk,0,0,&getLen,0.f);
*/

                        /* Load plugin data
plugin->dispatcher(plugin,effSetChunk,0,(VstInt32)tempLength,&buffer,0);
*/

                        ImGui::ColorEdit4(
                            "MyColor##track",
                            track.GetColor(),
                            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);

                        ImGui::SameLine();

                        if (_vstPluginLoader != nullptr)
                        {
                            if (ImGui::Button("Change plugin"))
                            {
                                auto plugin = _vstPluginLoader->LoadFromFileDialog();
                                if (plugin != nullptr)
                                {
                                    track.GetInstrument()->SetPlugin(plugin);
                                }
                            }
                            ImGui::SameLine();
                        }

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

                        ImGui::Text(
                            "%s by %s",
                            vstPlugin->getEffectName().c_str(),
                            vstPlugin->getVendorName().c_str());
                    }
                }
            }
        }

        auto trackId = std::get<uint32_t>(_tracks->GetActiveRegion());
        if (trackId != Track::Null && trackId == _tracks->GetActiveTrackId())
        {
            auto &track = _tracks->GetTrack(trackId);

            auto regionStart = std::get<std::chrono::milliseconds::rep>(_tracks->GetActiveRegion());
            if (track.Regions().find(regionStart) != track.Regions().end())
            {
                auto &region = track.GetRegion(regionStart);

                if (ImGui::CollapsingHeader((std::string("Region: ") + region.GetName()).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                {
                    if ((region.Length() % 4000) / 1000 > 0)
                    {
                        ImGui::Text("Length: %lu bars and %lu steps", (region.Length() / 4000), (region.Length() % 4000) / 1000);
                    }
                    else
                    {
                        ImGui::Text("Length: %lu bars", (region.Length() / 4000));
                    }

                    if (_editRegionName)
                    {
                        ImGui::SetKeyboardFocusHere();
                        if (ImGui::InputText("##editName", _editRegionNameBuffer, 128, ImGuiInputTextFlags_EnterReturnsTrue))
                        {
                            region.SetName(_editRegionNameBuffer);
                            _editRegionName = false;
                        }
                    }
                    else
                    {
                        ImGui::Text("%s", region.GetName().c_str());
                        if (ImGui::IsItemClicked())
                        {
                            _editRegionName = true;
                            strcpy_s(_editRegionNameBuffer, 128, region.GetName().c_str());
                        }
                    }
                }
            }
        }

        if (ImGui::CollapsingHeader("History", ImGuiTreeNodeFlags_DefaultOpen))
        {
            auto const *first = _state->_historyManager.FirstEntryInHistoryTrack();

            ImGui::BeginGroup();
            while (first != nullptr)
            {
                char undoTitle[64] = {0};
                sprintf_s(undoTitle, 64, "Undo %s", first->_title);
                ImGui::Selectable(undoTitle, first == _state->_historyManager.CurrentEntryInHistoryTrack());
                first = first->_nextEntry;
            }
            ImGui::EndGroup();
        }

        if (ImGui::CollapsingHeader("Quick Help", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Help!");
        }
    }

    ImGui::End();
}
