#ifndef TRACK_H
#define TRACK_H

#include "instrument.h"
#include "region.h"

#include <map>
#include <string>

class Track
{
    std::map<long, Region>::iterator _activeRegion;
    std::map<long, Region> _regions; // the int key of the map is the absolute start (in timestep=1000 per 1 bar) of the region from the beginning of the song

public:
    Instrument *_instrument = nullptr;
    std::string _name = "track";
    bool _muted = false;
    bool _readyForRecord = false;
    float _color[4];

    std::map<long, Region> const &Regions() const;

    Region &GetRegion(
        long at);

    void StartRecording();

    long StartNewRegion(
        long start);

    void RecordMidiEvent(
        long time,
        int noteNumber,
        bool onOff,
        int velocity);

    void AddRegion(
        long startAt,
        Region const &region);

    void RemoveRegion(
        long startAt);

    static std::map<long, Region>::iterator GetActiveRegionAt(
        std::map<long, Region> &regions,
        long time,
        long margin = 4000);
};

#endif // TRACK_H
