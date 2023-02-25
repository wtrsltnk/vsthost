#include "track.h"

#include "base64.h"
#include <spdlog/spdlog.h>

static uint32_t s_TrackCounter = 1;

Track::Track(
    uint32_t id)
    : _id(id)
{
    _color[0] = _color[1] = _color[2] = _color[3] = 0;
}

Track::Track()
    : Track(s_TrackCounter++)
{}

void Track::StartRecording()
{
    _activeRegion = -1;
}

std::chrono::milliseconds::rep Track::StartNewRegion(
    std::chrono::milliseconds::rep start)
{
    auto tmp = GetActiveRegionAt(_regions, start, 0);
    if (tmp != -1)
    {
        return tmp;
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
    if (_activeRegion == -1)
    {
        _activeRegion = GetActiveRegionAt(_regions, time);
    }

    if (_activeRegion == -1)
    {
        Region region;
        region.SetLength(4000);

        AddRegion(time - (time % 4000), region);

        _activeRegion = GetActiveRegionAt(_regions, time);
    }

    _regions[_activeRegion].AddEvent(time - _activeRegion, noteNumber, onOff, velocity);
}

void Track::SetName(
    const std::string &name)
{
    _name = name;
}

void Track::SetInstrument(
    std::shared_ptr<Instrument> instrument)
{
    this->_instrument = instrument;
}

void Track::DownloadInstrumentSettings()
{
    if (_instrument.get() == nullptr)
    {
        return;
    }

    _instrument->Lock();

    if (_instrument->Plugin() == nullptr)
    {
        _instrument->Unlock();
        return;
    }

    /* Save plugin data*/
    void *getLen;
    auto length = _instrument->Plugin()->dispatcher(effGetChunk, 0, 0, &getLen, 0.0f);
    auto data = reinterpret_cast<BYTE *>(getLen);
    _instrumentDataBase64 = base64_encode(&data[0], length);

    //spdlog::debug("downloading instrumentDataBase64: {0}", _instrumentDataBase64);

    _instrument->Unlock();
}

void Track::UploadInstrumentSettings()
{
    if (_instrument.get() == nullptr)
    {
        return;
    }

    _instrument->Lock();

    if (_instrument->Plugin() == nullptr)
    {
        _instrument->Unlock();
        return;
    }

    //spdlog::debug("uploading instrumentDataBase64: {0}", _instrumentDataBase64);

    auto data = base64_decode(_instrumentDataBase64);

    /* Load plugin data*/
    _instrument->Plugin()->dispatcher(effSetChunk, 0, (VstInt32)data.size(), data.data(), 0);

    _instrument->Unlock();
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
    const glm::vec4 &color)
{
    _color[0] = color[0];
    _color[1] = color[1];
    _color[2] = color[2];
    _color[3] = color[3];
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

Track::RegionCollection const &Track::Regions() const
{
    return _regions;
}

void Track::SetRegions(
    RegionCollection const &regions)
{
    _regions = regions;
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

std::chrono::milliseconds::rep Track::GetActiveRegionAt(
    std::map<std::chrono::milliseconds::rep, Region> &regions,
    std::chrono::milliseconds::rep time,
    std::chrono::milliseconds::rep margin)
{
    for (auto i = regions.begin(); i != regions.end(); ++i)
    {
        if (i->first <= time && (i->first + i->second.Length() + margin) >= time)
        {
            return i->first;
        }
    }

    return -1;
}
