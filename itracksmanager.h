#ifndef ITRACKSMANAGER_H
#define ITRACKSMANAGER_H

#include "itrack.h"

class ITracksManager
{
public:
    virtual ~ITracksManager() = default;

    virtual std::vector<ITrack *> GetTracks() = 0;
    virtual std::vector<Instrument *> GetInstruments() = 0;

    virtual ITrack *GetActiveTrack() = 0;
    virtual void SetActiveTrack(
        ITrack *track) = 0;

    virtual ITrack *GetSoloTrack() = 0;
    virtual void SetSoloTrack(
        ITrack *track) = 0;

    virtual std::tuple<ITrack *, long> &GetActiveRegion() = 0;
    virtual void SetActiveRegion(
        ITrack *track,
        long start) = 0;

    virtual ITrack *AddTrack(
        const std::string &name,
        Instrument *instrument = nullptr) = 0;

    virtual void RemoveTrack(
        ITrack *track) = 0;

    virtual void RemoveActiveRegion() = 0;

    virtual void CleanupInstruments() = 0;

    virtual void SendMidiNotesInSong(
        std::chrono::milliseconds::rep start,
        std::chrono::milliseconds::rep end) = 0;
};

#endif // ITRACKSMANAGER_H
