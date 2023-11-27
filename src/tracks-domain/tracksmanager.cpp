#include "tracksmanager.h"

#include <algorithm>
#include <exception>
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
        _soloTrack = 0;
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

void ColorConvertHSVtoRGB(
    float h,
    float s,
    float v,
    float &out_r,
    float &out_g,
    float &out_b)
{
    if (s == 0.0f)
    {
        // gray
        out_r = out_g = out_b = v;
        return;
    }

    h = fmodf(h, 1.0f) / (60.0f / 360.0f);
    int i = (int)h;
    float f = h - (float)i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));

    switch (i)
    {
        case 0:
            out_r = v;
            out_g = t;
            out_b = p;
            break;
        case 1:
            out_r = q;
            out_g = v;
            out_b = p;
            break;
        case 2:
            out_r = p;
            out_g = v;
            out_b = t;
            break;
        case 3:
            out_r = p;
            out_g = q;
            out_b = v;
            break;
        case 4:
            out_r = t;
            out_g = p;
            out_b = v;
            break;
        case 5:
        default:
            out_r = v;
            out_g = p;
            out_b = q;
            break;
    }
}

uint32_t TracksManager::AddTrack(
    const std::string &name,
    std::shared_ptr<Instrument> instrument)
{
    _instruments.push_back(instrument);

    Track newTrack;
    newTrack.SetInstrument(instrument);
    newTrack.SetName(name);
    newTrack.DownloadInstrumentSettings();

    float c[3]; // = ImColor::HSV(trackColorIndex++ * 0.05f, 0.6f, 0.6f);
    ColorConvertHSVtoRGB(trackColorIndex++ * 0.05f, 0.6f, 0.6f, c[0], c[1], c[2]);
    newTrack.SetColor(
        c[0],
        c[1],
        c[2],
        1.0f);

    _tracks.push_back(newTrack);

    _activeTrack = newTrack.Id();

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
    newi->SetInstrumentPlugin(nullptr);

    if (plugin != nullptr)
    {
        newi->SetInstrumentPlugin(std::make_unique<VstPlugin>());
        
        if (!newi->InstrumentPlugin()->init(plugin))
        {
            newi->SetInstrumentPlugin(nullptr);
        }
    }

    std::stringstream trackName;
    trackName << "Track " << (_tracks.size() + 1);

    return AddTrack(trackName.str(), std::shared_ptr<Instrument>(newi));
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
        
//        if (item->InstrumentPlugin() != nullptr)
//        {
//            item->SetInstrumentPlugin(nullptr);
//        }

        _instruments.pop_back();
    }
}

void TracksManager::SendMidiNotesInRegion(
    std::chrono::milliseconds::rep start,
    std::chrono::milliseconds::rep end)
{
    auto trackId = std::get<uint32_t>(activeRegion);

    if (trackId == Track::Null)
    {
        return;
    }

    if (trackId != _activeTrack)
    {
        return;
    }

    auto &track = GetTrack(trackId);

    if (track.GetInstrument() == nullptr)
    {
        return;
    }

    track.GetInstrument()->Lock();
    
    if (track.GetInstrument()->InstrumentPlugin() == nullptr)
    {
        track.GetInstrument()->Unlock();

        return;
    }

    auto regionStart = std::get<std::chrono::milliseconds::rep>(activeRegion);

    auto &region = track.GetRegion(regionStart);

    for (const auto &event : region.Events())
    {
        if ((event.first + regionStart) > end) continue;
        if ((event.first + regionStart) < start) continue;
        for (const auto &m : event.second)
        {
            track.GetInstrument()->InstrumentPlugin()->sendMidiNote(
                m.channel,
                m.num,
                m.value != 0,
                m.value);
        }
    }

    track.GetInstrument()->Unlock();
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

        track.GetInstrument()->Lock();
        
        if (track.GetInstrument()->InstrumentPlugin() == nullptr)
        {
            track.GetInstrument()->Unlock();

            continue;
        }

        for (const auto &region : track.Regions())
        {
            if (region.first > end) continue;
            if (region.first + region.second.Length() < start) continue;

            for (const auto &event : region.second.Events())
            {
                if ((event.first + region.first) > end) continue;
                if ((event.first + region.first) < start) continue;
                for (const auto &m : event.second)
                {
                    track.GetInstrument()->InstrumentPlugin()->sendMidiNote(
                        m.channel,
                        m.num,
                        m.value != 0,
                        m.value);
                }
            }
        }

        track.GetInstrument()->Unlock();
    }
}
