#ifndef TRACKSMANAGER_H
#define TRACKSMANAGER_H

#include "itracksmanager.h"
#include "track.h"

#include <vector>

class TracksManager :
    public ITracksManager
{
public:
    TracksManager();

    virtual std::vector<Track> &GetTracks() { return _tracks; }
    virtual void SetTracks(
        const std::vector<Track> &tracks);
    virtual std::vector<std::shared_ptr<Instrument>> &GetInstruments() { return _instruments; }

    virtual Track &GetTrack(
        uint32_t trackId);

    virtual uint32_t GetActiveTrackId();
    virtual void SetActiveTrack(
        uint32_t trackId);

    virtual uint32_t GetSoloTrack();
    virtual void SetSoloTrack(
        uint32_t trackId);

    virtual std::tuple<uint32_t, std::chrono::milliseconds::rep> &GetActiveRegion() { return activeRegion; }
    virtual void SetActiveRegion(
        uint32_t trackId,
        std::chrono::milliseconds::rep start);

    virtual uint32_t AddTrack(
        const std::string &name,
        std::shared_ptr<Instrument> instrument);

    virtual void RemoveTrack(
        uint32_t trackId);

    virtual std::shared_ptr<Instrument> GetInstrument(
        uint32_t trackId);

    virtual void RemoveActiveRegion();

    virtual void CleanupInstruments();

    virtual void SendMidiNotesInSong(
        std::chrono::milliseconds::rep start,
        std::chrono::milliseconds::rep end);

public:
    uint32_t AddVstTrack(
        const char *plugin = nullptr);

private:
    std::vector<Track> _tracks;
    std::vector<std::shared_ptr<Instrument>> _instruments;
    uint32_t _activeTrack = 0;
    uint32_t _soloTrack = 0;
    std::tuple<uint32_t, std::chrono::milliseconds::rep> activeRegion{Track::Null, -1};
};

#endif // TRACKSMANAGER_H
