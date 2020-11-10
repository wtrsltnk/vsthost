#ifndef TRACK_H
#define TRACK_H

#include "instrument.h"
#include "region.h"

#include <glm/glm.hpp>
#include <map>
#include <string>

class Track
{
public:
    typedef std::map<std::chrono::milliseconds::rep, Region> RegionCollection;

public:
    Track();
    virtual ~Track() = default;

    uint32_t Id() const { return _id; }

    const std::string &GetName() { return _name; }
    void SetName(
        std::string const &name);

    std::shared_ptr<Instrument> GetInstrument() { return _instrument; }
    void SetInstrument(
        std::shared_ptr<Instrument> instrument);

    bool IsMuted() { return _muted; }
    void Mute();
    void Unmute();
    void ToggleMuted();

    bool IsReadyForRecoding() { return _readyForRecord; }
    void SetReadyForRecording(
        bool ready);
    void ToggleReadyForRecording();

    float *GetColor() { return _color; }
    void SetColor(
        float color[]);
    void SetColor(
        float r,
        float g,
        float b,
        float a = 1.0f);
    void SetColor(
        const glm::vec4 &color);

    RegionCollection const &Regions() const;

    void SetRegions(
        RegionCollection const &regions);

    Region &GetRegion(
        std::chrono::milliseconds::rep at);

    void StartRecording();

    std::chrono::milliseconds::rep StartNewRegion(
        std::chrono::milliseconds::rep start);

    void RecordMidiEvent(
        std::chrono::milliseconds::rep time,
        int noteNumber,
        bool onOff,
        int velocity);

    void AddRegion(
        std::chrono::milliseconds::rep startAt,
        Region const &region);

    void RemoveRegion(
        std::chrono::milliseconds::rep startAt);

    static std::chrono::milliseconds::rep GetActiveRegionAt(
        std::map<std::chrono::milliseconds::rep, Region> &regions,
        std::chrono::milliseconds::rep time,
        std::chrono::milliseconds::rep margin = 4000);

    static const uint32_t Null = 0;

private:
    Track(
        uint32_t id);

    uint32_t _id;
    std::string _name = "track";
    std::shared_ptr<Instrument> _instrument;
    bool _muted = false;
    bool _readyForRecord = false;
    float _color[4];
    std::map<std::chrono::milliseconds::rep, Region> _regions; // the int key of the map is the absolute start (in timestep=1000 per 1 bar) of the region from the beginning of the song

    std::chrono::milliseconds::rep _activeRegion = -1;
};

#endif // TRACK_H
