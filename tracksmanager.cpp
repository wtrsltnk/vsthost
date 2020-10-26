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
    std::chrono::milliseconds::rep start)
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
    const char *plugin)
{
    std::stringstream instrumentName;
    instrumentName << "Instrument " << (tracks.size() + 1);

    auto newi = new Instrument();
    newi->SetName(instrumentName.str());
    newi->SetMidiChannel(0);
    newi->SetPlugin(nullptr);

    if (plugin != nullptr)
    {
        newi->SetPlugin(new VstPlugin());
        if (!newi->Plugin()->init(plugin))
        {
            auto tmp = newi->Plugin();
            newi->SetPlugin(nullptr);
            delete tmp;
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

    auto regionStart = std::get<std::chrono::milliseconds::rep>(activeRegion);

    track->RemoveRegion(regionStart);

    activeRegion = {nullptr, -1};
}

void TracksManager::CleanupInstruments()
{
    while (!instruments.empty())
    {
        auto item = instruments.back();
        if (item->Plugin() != nullptr)
        {
            auto tmp = item->Plugin();
            item->SetPlugin(nullptr);
            delete tmp;
        }
        instruments.pop_back();
        delete item;
    }
}

void TracksManager::SendMidiNotesInSong(
    std::chrono::milliseconds::rep start,
    std::chrono::milliseconds::rep end)
{
    for (auto track : GetTracks())
    {
        if (track->GetInstrument() == nullptr)
        {
            continue;
        }
        if (track->GetInstrument()->Plugin() == nullptr)
        {
            continue;
        }

        for (auto region : track->Regions())
        {
            if (region.first > end) continue;
            if (region.first + region.second.Length() < start) continue;

            for (auto event : region.second.Events())
            {
                if ((event.first + region.first) > end) continue;
                if ((event.first + region.first) < start) continue;
                for (auto m : event.second)
                {
                    track->GetInstrument()->Plugin()->sendMidiNote(
                        m.channel,
                        m.num,
                        m.value != 0,
                        m.value);
                }
            }
        }
    }
}
