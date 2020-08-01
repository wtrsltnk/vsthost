#include "track.h"

void Track::StartRecording()
{
    _activeRegion = _regions.end();
}

bool Track::StartNewRegion(
    long start)
{
    if (GetActiveRegionAt(_regions, start, 0) != _regions.end())
    {
        return false;
    }

    start = start - (start % 4000);

    Region region;
    region._length = 4000;

    _regions.insert(std::make_pair(start, region));

    return true;
}

void Track::RecordMidiEvent(
    long time,
    int noteNumber,
    bool onOff,
    int velocity)
{
    if (_activeRegion == _regions.end())
    {
        _activeRegion = GetActiveRegionAt(_regions, time);
    }

    if (_activeRegion == _regions.end())
    {
        Region region;
        region._length = 4000;

        _regions.insert(std::make_pair(time - (time % 4000), region));

        _activeRegion = GetActiveRegionAt(_regions, time);
    }

    _activeRegion->second.AddEvent(time - _activeRegion->first, noteNumber, onOff, velocity);
}

std::map<long, Region>::iterator Track::GetActiveRegionAt(
    std::map<long, Region> &regions,
    long time,
    long margin)
{
    for (auto i = regions.begin(); i != regions.end(); ++i)
    {
        if (i->first <= time && (i->first + i->second._length + margin) >= time)
        {
            return i;
        }
    }

    return regions.end();
}
