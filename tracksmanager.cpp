#include "tracksmanager.h"

#include <algorithm>
#include <imgui.h>
#include <iostream>
#include <sstream>
#include <tuple>

static int trackColorIndex = 0;

TracksManager::TracksManager()
{
}

void TracksManager::SetActiveTrack(
    ITrack *track)
{
    if (track != nullptr && std::find(tracks.begin(), tracks.end(), track) == tracks.end())
    {
        return;
    }

    activeTrack = track;
}

void TracksManager::SetSoloTrack(
    ITrack *track)
{
    if (track != nullptr && std::find(tracks.begin(), tracks.end(), track) == tracks.end())
    {
        return;
    }

    soloTrack = track;
}

void TracksManager::SetActiveRegion(
    ITrack *track,
    long start)
{
    if (track != nullptr && std::find(tracks.begin(), tracks.end(), track) == tracks.end())
    {
        return;
    }

    activeRegion = std::tuple<ITrack *, long>(track, start);
}

ITrack *TracksManager::AddTrack(
    const std::string &name,
    Instrument *instrument)
{
    auto newTrack = new Track();
    newTrack->SetInstrument(instrument);
    newTrack->SetName(name);

    auto c = ImColor::HSV(trackColorIndex++ * 0.05f, 0.6f, 0.6f);
    newTrack->SetColor(
        c.Value.x,
        c.Value.y,
        c.Value.z,
        c.Value.w);

    tracks.push_back(newTrack);

    return newTrack;
}

ITrack *TracksManager::AddVstTrack(
    wchar_t const *plugin)
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
        if (!newi->_plugin->init(plugin))
        {
            delete newi->_plugin;
            newi->_plugin = nullptr;
        }
    }

    instruments.push_back(newi);

    std::stringstream trackName;
    trackName << "Track " << (tracks.size() + 1);

    return AddTrack(trackName.str(), newi);
}

void TracksManager::RemoveTrack(
    ITrack *track)
{
    if (track == nullptr)
    {
        return;
    }

    if (track == activeTrack)
    {
        activeTrack = nullptr;
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

void TracksManager::RemoveActiveRegion()
{
    auto track = std::get<ITrack *>(activeRegion);

    if (track == nullptr)
    {
        return;
    }

    auto regionStart = std::get<long>(activeRegion);

    track->RemoveRegion(regionStart);
}

void TracksManager::CleanupInstruments()
{
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
}
