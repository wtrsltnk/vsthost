#ifndef TRACKSMANAGER_H
#define TRACKSMANAGER_H

#include "track.h"

#include <vector>

class TracksManager
{
public:
    std::vector<Instrument *> instruments;
    std::vector<Track *> tracks;
    Track *activeTrack = nullptr;
    Track *soloTrack = nullptr;
    std::tuple<Track *, long> activeRegion{nullptr, -1  };

    TracksManager();

    Track *AddVstTrack(
        wchar_t const *plugin = nullptr);

    void RemoveTrack(Track *track);

    void RemoveActiveRegion();

    void CleanupInstruments();
};

#endif // TRACKSMANAGER_H
