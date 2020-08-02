#include "tracksmanager.h"

#include <imgui.h>
#include <sstream>

static int trackColorIndex = 0;

TracksManager::TracksManager()
{
}

Track *TracksManager::AddVstTrack(
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

    auto newTrack = new Track();
    newTrack->_instrument = newi;
    newTrack->_name = trackName.str();

    auto c = ImColor::HSV(trackColorIndex++ * 0.05f, 0.6f, 0.6f);
    newTrack->_color[0] = c.Value.x;
    newTrack->_color[1] = c.Value.y;
    newTrack->_color[2] = c.Value.z;
    newTrack->_color[3] = c.Value.w;

    tracks.push_back(newTrack);

    return newTrack;
}

void TracksManager::RemoveTrack(Track *track)
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
    auto track = std::get<Track *>(activeRegion);

    if (track == nullptr)
    {
        return;
    }

    auto regionStart = std::get<long>(activeRegion);

    track->_regions.erase(regionStart);
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
