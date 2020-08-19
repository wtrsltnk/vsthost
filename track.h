#ifndef TRACK_H
#define TRACK_H

#include "instrument.h"
#include "region.h"

#include "trackseditor.h"

#include <map>
#include <string>

class Track : public ITrack
{
    std::string _name = "track";
    Instrument *_instrument = nullptr;
    bool _muted = false;
    bool _readyForRecord = false;
    float _color[4];

    std::map<long, Region>::iterator _activeRegion;
    std::map<long, Region> _regions; // the int key of the map is the absolute start (in timestep=1000 per 1 bar) of the region from the beginning of the song

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

    virtual std::map<long, Region> const &Regions() const;

    virtual Region &GetRegion(
        long at);

    virtual void StartRecording();

    virtual long StartNewRegion(
        long start);

    virtual void RecordMidiEvent(
        long time,
        int noteNumber,
        bool onOff,
        int velocity);

    virtual void AddRegion(
        long startAt,
        Region const &region);

    virtual void RemoveRegion(
        long startAt);

    static std::map<long, Region>::iterator GetActiveRegionAt(
        std::map<long, Region> &regions,
        long time,
        long margin = 4000);
};

#endif // TRACK_H
