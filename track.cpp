#include "track.h"

void Track::StartRecording()
{
    _activeRegion = _regions.end();
}

long Track::StartNewRegion(
    long start)
{
    if (GetActiveRegionAt(_regions, start, 0) != _regions.end())
    {
        return -1;
    }

    start = start - (start % 4000);

    Region region;
    region._length = 4000;

    AddRegion(start, region);

    return start;
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

        AddRegion(time - (time % 4000), region);

        _activeRegion = GetActiveRegionAt(_regions, time);
    }

    _activeRegion->second.AddEvent(time - _activeRegion->first, noteNumber, onOff, velocity);
}

std::map<long, Region> const &Track::Regions() const
{
    return _regions;
}

Region &Track::GetRegion(
    long at)
{
    return _regions[at];
}

void Track::AddRegion(
    long startAt,
    Region const &region)
{
    _regions.insert(std::make_pair(startAt, region));
}

void Track::RemoveRegion(
    long startAt)
{
    _regions.erase(startAt);
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
