#include "track.h"

void Track::RecordMidiEvent(
    long time,
    int noteNumber,
    bool onOff,
    int velocity)
{
    auto activeRegion = GetActiveRegionAt(_regions, time);
    if (activeRegion == _regions.end())
    {
        Region region;
        region._length = 4000;

        _regions.insert(std::make_pair(time - (time % 4000), region));

        activeRegion = GetActiveRegionAt(_regions, time);
    }

    activeRegion->second.AddEvent(time - activeRegion->first, noteNumber, onOff, velocity);
}

std::map<long, Region>::iterator Track::GetActiveRegionAt(
    std::map<long, Region> &regions,
    long time)
{
    for (auto i = regions.begin(); i != regions.end(); ++i)
    {
        if (i->first <= time && (i->first + i->second._length + 4000) >= time)
        {
            return i;
        }
    }

    return regions.end();
}
