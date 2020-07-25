#include "tracksmanager.h"

#include <imgui.h>
#include <sstream>

TracksManager::TracksManager()
{
}

Track *TracksManager::addVstTrack(
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
        newi->_plugin->init(plugin);
    }

    instruments.push_back(newi);

    std::stringstream trackName;
    trackName << "Track " << (tracks.size() + 1);

    auto newt = new Track();
    newt->_instrument = newi;
    newt->_name = trackName.str();

    auto c = ImColor::HSV(tracks.size() * 0.05f, 0.6f, 0.6f);
    newt->_color[0] = c.Value.x;
    newt->_color[1] = c.Value.y;
    newt->_color[2] = c.Value.z;
    newt->_color[3] = c.Value.w;
    tracks.push_back(newt);

    return newt;
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
void TracksManager::removeTrack(Track *track)
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
