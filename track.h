#ifndef TRACK_H
#define TRACK_H

#include "instrument.h"
#include "region.h"

#include "trackseditor.h"

#include <map>
#include <string>

class Track :
    public ITrack
{
public:
    virtual ~Track() = default;

    virtual const std::string &GetName() { return _name; }
    virtual void SetName(
        std::string const &name);

    virtual Instrument *GetInstrument() { return _instrument; }
    virtual void SetInstrument(
        Instrument *instrument);

    virtual bool IsMuted() { return _muted; }
    virtual void Mute();
    virtual void Unmute();
    virtual void ToggleMuted();

    virtual bool IsReadyForRecoding() { return _readyForRecord; }
    virtual void SetReadyForRecording(
        bool ready);
    virtual void ToggleReadyForRecording();

    virtual float *GetColor() { return _color; }
    virtual void SetColor(
        float color[]);
    virtual void SetColor(
        float r,
        float g,
        float b,
        float a = 1.0f);

    virtual RegionCollection const &Regions() const;

    virtual Region &GetRegion(
        std::chrono::milliseconds::rep at);

    virtual void StartRecording();

    virtual std::chrono::milliseconds::rep StartNewRegion(
        std::chrono::milliseconds::rep start);

    virtual void RecordMidiEvent(
        std::chrono::milliseconds::rep time,
        int noteNumber,
        bool onOff,
        int velocity);

    virtual void AddRegion(
        std::chrono::milliseconds::rep startAt,
        Region const &region);

    virtual void RemoveRegion(
        std::chrono::milliseconds::rep startAt);

    static std::map<std::chrono::milliseconds::rep, Region>::iterator GetActiveRegionAt(
        std::map<std::chrono::milliseconds::rep, Region> &regions,
        std::chrono::milliseconds::rep time,
        std::chrono::milliseconds::rep margin = 4000);

private:
    std::string _name = "track";
    Instrument *_instrument = nullptr;
    bool _muted = false;
    bool _readyForRecord = false;
    float _color[4];
    std::map<std::chrono::milliseconds::rep, Region> _regions; // the int key of the map is the absolute start (in timestep=1000 per 1 bar) of the region from the beginning of the song

    std::map<std::chrono::milliseconds::rep, Region>::iterator _activeRegion;
};

#endif // TRACK_H
