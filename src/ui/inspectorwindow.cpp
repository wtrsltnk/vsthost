#include "inspectorwindow.h"

#include "../historymanager.h"
#include "IconsFontaudio.h"
#include "instrument.h"
#include "ipluginservice.h"

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
    IPluginService *loader)
{
    _vstPluginLoader = loader;
}

std::vector<struct PluginDescription> InspectorWindow::PluginLibrary(
    const char *id,
    std::function<void(const std::shared_ptr<class VstPlugin> &)> onPLuginSelected,
    std::vector<struct PluginDescription> &plugins,
    std::function<bool(const struct PluginDescription &)> filter)
{
    static std::string vendorNameFilter;

    if (ImGui::BeginPopup(id))
    {
        if (ImGui::Button("Add form disk"))
        {
            auto plugin = _vstPluginLoader->LoadFromFileDialog();
            if (plugin != nullptr)
            {
                plugins = _vstPluginLoader->ListPlugins(filter);
            }
        }

        ImGui::Text("Filter by vendor: %s", vendorNameFilter.c_str());

        if (!vendorNameFilter.empty())
        {
            ImGui::SameLine();
            if (ImGui::SmallButton("X"))
            {
                vendorNameFilter = "";
                plugins = _vstPluginLoader->ListPlugins(filter);
            }
        }

        if (ImGui::BeginTable("table1", 6, 0, ImVec2(700, 200)))
        {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.3f);
            ImGui::TableSetupColumn("Vendor", ImGuiTableColumnFlags_WidthStretch, 0.2f);
            ImGui::TableSetupColumn("Ins", ImGuiTableColumnFlags_WidthStretch, 0.1f);
            ImGui::TableSetupColumn("Outs", ImGuiTableColumnFlags_WidthStretch, 0.1f);
            ImGui::TableSetupColumn("Effect?", ImGuiTableColumnFlags_WidthStretch, 0.15f);
            ImGui::TableSetupColumn("Editor?", ImGuiTableColumnFlags_WidthStretch, 0.15f);
            ImGui::TableHeadersRow();

            std::string changeVendorNameFilter;
            for (size_t row = 0; row < plugins.size(); row++)
            {
                ImGui::PushID(row);

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                if (ImGui::SmallButton(plugins[row].effectName.c_str()))
                {
                    auto plugin = _vstPluginLoader->LoadPlugin(plugins[row].path);
                    if (plugin != nullptr)
                    {
                        onPLuginSelected(plugin);

                        plugins = _vstPluginLoader->ListPlugins(filter);
                        ImGui::CloseCurrentPopup();
                    }
                }

                ImGui::TableSetColumnIndex(1);
                if (ImGui::SmallButton(plugins[row].vendorName.c_str()))
                {
                    changeVendorNameFilter = plugins[row].vendorName;
                }

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%d", plugins[row].inputCount);

                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%d", plugins[row].outputCount);

                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%s", plugins[row].isSynth ? "no" : "yes");

                ImGui::TableSetColumnIndex(5);
                ImGui::Text(plugins[row].hasEditor ? "yes" : "no");

                ImGui::PopID();
            }
            ImGui::EndTable();

            if (!changeVendorNameFilter.empty())
            {
                vendorNameFilter = changeVendorNameFilter;
                plugins = _vstPluginLoader->ListPlugins(
                    [&](struct PluginDescription d) {
                        return filter(d) && vendorNameFilter == d.vendorName;
                    });
            }
        }

        ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - 80);
        if (ImGui::Button("Close", ImVec2(80, 0)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

    return plugins;
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

        if (_midiIn != nullptr)
        {
            bool midiPortsAvailable = _midiIn->getPortCount() > 0;

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
                    auto &vstPlugin = track.GetInstrument()->InstrumentPlugin();

                    if (vstPlugin == nullptr)
                    {
                        ImGui::ColorEdit4(
                            "MyColor##track",
                            track.GetColor(),
                            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);

                        ImGui::SameLine();

                        if (_vstPluginLoader != nullptr && ImGui::Button("Load plugin"))
                        {
                            ImGui::OpenPopup("InstrumentBrowser");
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
                                ImGui::OpenPopup("InstrumentBrowser");
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

                    static auto plugins = _vstPluginLoader->ListPlugins([](struct PluginDescription d) { return d.isSynth; });

                    plugins = PluginLibrary(
                        "InstrumentBrowser",
                        [&](const std::shared_ptr<class VstPlugin> &plugin) {
                            _state->_historyManager.AddEntry("Load different plugin");
                            track.GetInstrument()->SetInstrumentPlugin(plugin);
                        },
                        plugins,
                        [&](struct PluginDescription d) { return d.isSynth; });
                }
            }

            auto stripWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().FramePadding.y) / 2;
            auto stripHeight = ImGui::GetContentRegionAvail().y - (2 * ImGui::GetStyle().FramePadding.y) - (2 * ImGui::GetStyle().ItemSpacing.y);

            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0.1f));
            ImGui::BeginChild("##trackStrip", ImVec2(stripWidth, stripHeight));

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 1));
            for (int i = 0; i < MAX_EFFECT_PLUGINS; i++)
            {
                ImGui::PushID(i);
                auto effect = track.GetInstrument()->EffectPlugin(i);
                if (effect == nullptr)
                {
                    if (ImGui::Button("No Effect", ImVec2(stripWidth, 0)))
                    {
                        ImGui::OpenPopup("EffectBrowser");
                    }

                    static auto plugins = _vstPluginLoader->ListPlugins([](struct PluginDescription d) { return !d.isSynth; });

                    plugins = PluginLibrary(
                        "EffectBrowser",
                        [&](const std::shared_ptr<class VstPlugin> &plugin) {
                            _state->_historyManager.AddEntry("Load different effect");
                            track.GetInstrument()->SetEffectPlugin(i, plugin);
                        },
                        plugins,
                        [&](struct PluginDescription d) { return !d.isSynth; });
                }
                else
                {
                    const auto ih = ImGui::GetTextLineHeight() + ImGui::GetStyle().FramePadding.y * 2;

                    if (ImGui::Button(effect->Title(), ImVec2(stripWidth - ih - 2, ih)))
                    {
                        track.DownloadEffectSettings(i);
                        effect->openEditor(nullptr);
                    }
                    ImGui::SameLine(0.0f, 2.0f);
                    if (ImGui::Button("X", ImVec2(ih, ih)))
                    {
                        track.GetInstrument()->SetEffectPlugin(i, nullptr);
                    }
                }
                ImGui::PopID();
            }
            ImGui::PopStyleVar();

            ImGui::SetCursorPos(ImVec2(ImGui::GetStyle().ItemSpacing.x, stripHeight - 60 - ImGui::GetStyle().ItemSpacing.y));
            ImGui::Button("M", ImVec2(stripWidth / 3, 0));
            ImGui::SetCursorPos(ImVec2(stripWidth - (stripWidth / 3) - ImGui::GetStyle().ItemSpacing.x, stripHeight - 60 - ImGui::GetStyle().ItemSpacing.y));
            ImGui::Button("S", ImVec2(stripWidth / 3, 0));
            ImGui::Button(track.GetName().c_str(), ImVec2(stripWidth, 0));
            ImGui::EndChild();

            ImGui::SameLine();

            ImGui::BeginChild("##mainStrip", ImVec2(stripWidth, stripHeight));
            ImGui::SetCursorPos(ImVec2(ImGui::GetStyle().ItemSpacing.x, stripHeight - 60 - ImGui::GetStyle().ItemSpacing.y));
            ImGui::Button("M", ImVec2(stripWidth / 3, 0));
            ImGui::Button("Output", ImVec2(stripWidth, 0));
            ImGui::EndChild();
            ImGui::PopStyleColor();
        }
    }

    ImGui::End();
}
