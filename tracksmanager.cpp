#include "tracksmanager.h"

#include <algorithm>
#include <exception>
#include <imgui.h>
#include <iostream>
#include <spdlog/spdlog.h>
#include <sstream>
#include <tuple>

static int trackColorIndex = 0;

TracksManager::TracksManager() = default;

void TracksManager::SetTracks(
    const std::vector<Track> &tracks)
{
    _tracks = tracks;
}

bool DoesTrackIdExist(
    std::vector<Track> &tracks,
    uint32_t trackId)
{
    return std::any_of(
        tracks.begin(),
        tracks.end(),
        [&](const Track &x) {
            return x.Id() == trackId;
        });
}

Track &TracksManager::GetTrack(
    uint32_t trackId)
{
    if (!DoesTrackIdExist(_tracks, trackId))
    {
        spdlog::error("trackId {0} does not exist", trackId);

        throw std::out_of_range("trackId does not exist");
    }

    return *std::find_if(
        _tracks.begin(),
        _tracks.end(),
        [&](const Track &x) {
            return x.Id() == trackId;
        });
}

uint32_t TracksManager::GetActiveTrackId()
{
    if (!DoesTrackIdExist(_tracks, _activeTrack))
    {
        _activeTrack = Track::Null;
    }

    return _activeTrack;
}

void TracksManager::SetActiveTrack(
    uint32_t trackId)
{
    if (!DoesTrackIdExist(_tracks, trackId))
    {
        return;
    }

    _activeTrack = trackId;
}

uint32_t TracksManager::GetSoloTrack()
{
    if (!DoesTrackIdExist(_tracks, _soloTrack))
    {
        _soloTrack = Track::Null;
    }

    return _soloTrack;
}

void TracksManager::SetSoloTrack(
    uint32_t trackId)
{
    if (!DoesTrackIdExist(_tracks, trackId))
    {
        return;
    }

    _soloTrack = trackId;
}

void TracksManager::SetActiveRegion(
    uint32_t trackId,
    std::chrono::milliseconds::rep start)
{
    if (!DoesTrackIdExist(_tracks, trackId))
    {
        return;
    }

    activeRegion = std::tuple<uint32_t, std::chrono::milliseconds::rep>(trackId, start);
}

uint32_t TracksManager::AddTrack(
    const std::string &name,
    std::shared_ptr<Instrument> instrument)
{
    _instruments.push_back(instrument);

    Track newTrack;
    newTrack.SetInstrument(instrument);
    newTrack.SetName(name);

    auto c = ImColor::HSV(trackColorIndex++ * 0.05f, 0.6f, 0.6f);
    newTrack.SetColor(
        c.Value.x,
        c.Value.y,
        c.Value.z,
        c.Value.w);

    _tracks.push_back(newTrack);

    return newTrack.Id();
}

uint32_t TracksManager::AddVstTrack(
    const char *plugin)
{
    std::stringstream instrumentName;
    instrumentName << "Instrument " << (_tracks.size() + 1);

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

    _instruments.push_back(std::shared_ptr<Instrument>(newi));

    std::stringstream trackName;
    trackName << "Track " << (_tracks.size() + 1);

    return AddTrack(trackName.str(), _instruments.back());
}

void TracksManager::RemoveTrack(
    uint32_t trackId)
{
    if (trackId == Track::Null)
    {
        return;
    }

    if (trackId == _activeTrack)
    {
        _activeTrack = Track::Null;
    }

    auto found = std::find_if(
        _tracks.begin(),
        _tracks.end(),
        [&](const Track &x) {
            return x.Id() == trackId;
        });

    if (found != _tracks.end())
    {
        _tracks.erase(found);
    }
}

std::shared_ptr<Instrument> TracksManager::GetInstrument(
    uint32_t trackId)
{
    auto found = std::find_if(
        _tracks.begin(),
        _tracks.end(),
        [&](const Track &x) {
            return x.Id() == trackId;
        });

    if (found == _tracks.end())
    {
        return nullptr;
    }

    return (*found).GetInstrument();
}

void TracksManager::RemoveActiveRegion()
{
    auto trackId = std::get<uint32_t>(activeRegion);

    if (trackId == Track::Null)
    {
        return;
    }

    auto &track = GetTrack(trackId);

    auto regionStart = std::get<std::chrono::milliseconds::rep>(activeRegion);

    track.RemoveRegion(regionStart);

    activeRegion = {Track::Null, -1};
}

void TracksManager::CleanupInstruments()
{
    while (!_instruments.empty())
    {
        auto item = _instruments.back();
        if (item->Plugin() != nullptr)
        {
            auto tmp = item->Plugin();
            item->SetPlugin(nullptr);
            delete tmp;
        }
        _instruments.pop_back();
    }
}

void TracksManager::SendMidiNotesInSong(
    std::chrono::milliseconds::rep start,
    std::chrono::milliseconds::rep end)
{
    for (auto &track : GetTracks())
    {
        if (track.GetInstrument() == nullptr)
        {
            continue;
        }
        if (track.GetInstrument()->Plugin() == nullptr)
        {
            continue;
        }

        for (auto region : track.Regions())
        {
            if (region.first > end) continue;
            if (region.first + region.second.Length() < start) continue;

            for (auto event : region.second.Events())
            {
                if ((event.first + region.first) > end) continue;
                if ((event.first + region.first) < start) continue;
                for (auto m : event.second)
                {
                    track.GetInstrument()->Plugin()->sendMidiNote(
                        m.channel,
                        m.num,
                        m.value != 0,
                        m.value);
                }
            }
        }
    }
}
