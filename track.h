#ifndef TRACK_H
#define TRACK_H

#include "instrument.h"
#include "region.h"

#include <map>
#include <string>

class Track
{
public:
    Instrument *_instrument = nullptr;
    std::map<long, Region> _regions; // the int key of the map is the absolute start (in timestep=1000 per 1 bar) of the region from the beginning of the song
    std::string _name;
    bool _muted = false;
    bool _readyForRecord = false;
    float _color[4];

    void RecordMidiEvent(
        long time,
        int noteNumber,
        bool onOff,
        int velocity);

    static std::map<long, Region>::iterator GetActiveRegionAt(
        std::map<long, Region> &regions,
        long time);
};

#endif // TRACK_H
