#ifndef ITRACKSMANAGER_H
#define ITRACKSMANAGER_H

#include <track.h>

class ITracksManager
{
public:
    virtual ~ITracksManager() = default;

    virtual std::vector<Track> &GetTracks() = 0;
    virtual void SetTracks(
        const std::vector<Track> &tracks) = 0;
    virtual std::vector<std::shared_ptr<Instrument>> &GetInstruments() = 0;

    virtual Track &GetTrack(
        uint32_t trackId) = 0;

    virtual uint32_t GetActiveTrackId() = 0;
    virtual void SetActiveTrack(
        uint32_t trackId) = 0;

    virtual uint32_t GetSoloTrack() = 0;
    virtual void SetSoloTrack(
        uint32_t trackId) = 0;

    virtual std::tuple<uint32_t, std::chrono::milliseconds::rep> &GetActiveRegion() = 0;
    virtual void SetActiveRegion(
        uint32_t trackId,
        std::chrono::milliseconds::rep start) = 0;

    virtual uint32_t AddTrack(
        const std::string &name,
        std::shared_ptr<Instrument> instrument) = 0;

    virtual void RemoveTrack(
        uint32_t trackId) = 0;

    virtual std::shared_ptr<Instrument> GetInstrument(
        uint32_t trackId) = 0;

    virtual void RemoveActiveRegion() = 0;

    virtual void CleanupInstruments() = 0;

    virtual void SendMidiNotesInSong(
        std::chrono::milliseconds::rep start,
        std::chrono::milliseconds::rep end) = 0;

    virtual void SendMidiNotesInRegion(
        std::chrono::milliseconds::rep start,
        std::chrono::milliseconds::rep end) = 0;
};

#endif // ITRACKSMANAGER_H
