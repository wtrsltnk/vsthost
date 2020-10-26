#include "track.h"

void Track::StartRecording()
{
    _activeRegion = _regions.end();
}

std::chrono::milliseconds::rep Track::StartNewRegion(
    std::chrono::milliseconds::rep start)
{
    if (GetActiveRegionAt(_regions, start, 0) != _regions.end())
    {
        return -1;
    }

    start = start - (start % 4000);

    Region region;
    region.SetLength(4000);

    AddRegion(start, region);

    return start;
}

void Track::RecordMidiEvent(
    std::chrono::milliseconds::rep time,
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
        region.SetLength(4000);

        AddRegion(time - (time % 4000), region);

        _activeRegion = GetActiveRegionAt(_regions, time);
    }

    _activeRegion->second.AddEvent(time - _activeRegion->first, noteNumber, onOff, velocity);
}

void Track::SetName(
    std::string const &name)
{
    _name = name;
}

void Track::SetInstrument(
    Instrument *instrument)
{
    this->_instrument = instrument;
}

void Track::Mute()
{
    _muted = true;
}

void Track::Unmute()
{
    _muted = false;
}

void Track::ToggleMuted()
{
    _muted = !_muted;
}

void Track::SetReadyForRecording(
    bool ready)
{
    _readyForRecord = ready;
}

void Track::ToggleReadyForRecording()
{
    _readyForRecord = !_readyForRecord;
}

void Track::SetColor(
    float color[])
{
    _color[0] = color[0];
    _color[1] = color[1];
    _color[2] = color[2];
    _color[3] = color[3];
}

void Track::SetColor(
    float r,
    float g,
    float b,
    float a)
{
    _color[0] = r;
    _color[1] = g;
    _color[2] = b;
    _color[3] = a;
}

void Track::SetColor(
    const glm::vec4 &color)
{
    _color[0] = color.r;
    _color[1] = color.g;
    _color[2] = color.b;
    _color[3] = color.a;
}

ITrack::RegionCollection const &Track::Regions() const
{
    return _regions;
}

Region &Track::GetRegion(
    std::chrono::milliseconds::rep at)
{
    return _regions[at];
}

void Track::AddRegion(
    std::chrono::milliseconds::rep startAt,
    Region const &region)
{
    _regions.insert(std::make_pair(startAt, region));
}

void Track::RemoveRegion(
    std::chrono::milliseconds::rep startAt)
{
    _regions.erase(startAt);
}

std::map<std::chrono::milliseconds::rep, Region>::iterator Track::GetActiveRegionAt(
    std::map<std::chrono::milliseconds::rep, Region> &regions,
    std::chrono::milliseconds::rep time,
    std::chrono::milliseconds::rep margin)
{
    for (auto i = regions.begin(); i != regions.end(); ++i)
    {
        if (i->first <= time && (i->first + i->second.Length() + margin) >= time)
        {
            return i;
        }
    }

    return regions.end();
}
