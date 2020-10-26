#ifndef TRACKSMANAGER_H
#define TRACKSMANAGER_H

#include "track.h"

#include <vector>

class TracksManager :
    public ITracksManager
{
public:
    TracksManager();

    virtual std::vector<ITrack *> GetTracks() { return tracks; }
    virtual std::vector<Instrument *> GetInstruments() { return instruments; }

    virtual ITrack *GetActiveTrack() { return activeTrack; }
    virtual void SetActiveTrack(
        ITrack *track);

    virtual ITrack *GetSoloTrack() { return soloTrack; }
    virtual void SetSoloTrack(
        ITrack *track);

    virtual std::tuple<ITrack *, std::chrono::milliseconds::rep> &GetActiveRegion() { return activeRegion; }
    virtual void SetActiveRegion(
        ITrack *track,
        std::chrono::milliseconds::rep start);

    virtual ITrack *AddTrack(
        const std::string &name,
        Instrument *instrument = nullptr);

    virtual void RemoveTrack(
        ITrack *track);

    virtual void RemoveActiveRegion();

    virtual void CleanupInstruments();

    virtual void SendMidiNotesInSong(
        std::chrono::milliseconds::rep start,
        std::chrono::milliseconds::rep end);

public:
    ITrack *AddVstTrack(
        const char *plugin = nullptr);

private:
    std::vector<ITrack *> tracks;
    std::vector<Instrument *> instruments;
    ITrack *activeTrack = nullptr;
    ITrack *soloTrack = nullptr;
    std::tuple<ITrack *, std::chrono::milliseconds::rep> activeRegion{nullptr, -1};
};

#endif // TRACKSMANAGER_H
