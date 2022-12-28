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

        if (ImGui::CollapsingHeader("Quick Help", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Help!");
        }

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
                        ImGui::Text("Length: %lld bars and %lld steps", (region.Length() / 4000), (region.Length() % 4000) / 1000);
                    }
                    else
                    {
                        ImGui::Text("Length: %lld bars", (region.Length() / 4000));
                    }

                    if (_editRegionName)
                    {
                        ImGui::SetKeyboardFocusHere();
                        if (ImGui::InputText("##editName", _editRegionNameBuffer, 128, ImGuiInputTextFlags_EnterReturnsTrue))
                        {
                            _state->_historyManager.AddEntry("Change region name");
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

        if (_tracks->GetActiveTrackId() != Track::Null)
        {
            auto &track = _tracks->GetTrack(_tracks->GetActiveTrackId());

            if (ImGui::CollapsingHeader((std::string("Track: ") + track.GetName()).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (track.GetInstrument() != nullptr)
                {
                    auto &vstPlugin = track.GetInstrument()->Plugin();

                    if (vstPlugin == nullptr)
                    {
                        ImGui::ColorEdit4(
                            "MyColor##track",
                            track.GetColor(),
                            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);

                        ImGui::SameLine();

                        if (_vstPluginLoader != nullptr && ImGui::Button("Load plugin"))
                        {
                            auto plugin = _vstPluginLoader->LoadFromFileDialog();

                            if (plugin != nullptr)
                            {
                                _state->_historyManager.AddEntry("Load plugin");
                                track.GetInstrument()->SetPlugin(std::move(plugin));
                            }
                        }
                    }
                    else
                    {
                        ImGui::ColorEdit4(
                            "MyColor##track",
                            track.GetColor(),
                            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);

                        ImGui::SameLine();

                        if (_vstPluginLoader != nullptr)
                        {
                            if (ImGui::Button("Load different plugin"))
                            {
                                auto plugin = _vstPluginLoader->LoadFromFileDialog();
                                if (plugin != nullptr)
                                {
                                    _state->_historyManager.AddEntry("Load different plugin");
                                    track.GetInstrument()->SetPlugin(std::move(plugin));
                                }
                            }
                            ImGui::SameLine();
                        }

                        if (!vstPlugin->isEditorOpen())
                        {
                            if (ImGui::Button("Open plugin editor"))
                            {
                                track.DownloadInstrumentSettings();
                                vstPlugin->openEditor(nullptr);
                            }
                        }
                        else
                        {
                            if (ImGui::Button("Close plugin editor"))
                            {
                                _state->_historyManager.AddEntry("Close plugin editor");
                                vstPlugin->closeEditor();
                                track.DownloadInstrumentSettings();
                            }
                        }

                        ImGui::Text(
                            "%s by %s",
                            vstPlugin->getEffectName().c_str(),
                            vstPlugin->getVendorName().c_str());
                    }
                }
            }

            auto stripWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().FramePadding.y) / 2;
            auto stripHeight = ImGui::GetContentRegionAvail().y - (2 * ImGui::GetStyle().FramePadding.y) - (2 * ImGui::GetStyle().ItemSpacing.y);

            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0.1f));
            ImGui::BeginChild("##trackStrip", ImVec2(stripWidth, stripHeight));
            ImGui::SetCursorPos(ImVec2(0, stripHeight - 30));
            ImGui::Button(track.GetName().c_str(), ImVec2(stripWidth, 0));
            ImGui::EndChild();

            ImGui::SameLine();

            ImGui::BeginChild("##mainStrip", ImVec2(stripWidth, stripHeight));
            ImGui::SetCursorPos(ImVec2(0, stripHeight - 30));
            ImGui::Button("Stereo Out", ImVec2(stripWidth, 0));
            ImGui::EndChild();
            ImGui::PopStyleColor();
        }
    }

    ImGui::End();
}
