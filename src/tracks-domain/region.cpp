#include "region.h"

const std::string &Region::GetName() const
{
    return _name;
}

void Region::SetName(
    const std::string &name)
{
    _name = name;
}

std::chrono::milliseconds::rep Region::Length() const
{
    return _length;
}

void Region::SetLength(
    std::chrono::milliseconds::rep length)
{
    _length = length;
}

const MidiEvent::CollectionInTime &Region::Events() const
{
    return _events;
}

uint32_t Region::GetMinNote() const
{
    return _minNote;
}

uint32_t Region::GetMaxNote() const
{
    return _maxNote;
}

void Region::AddEvent(
    std::chrono::milliseconds::rep time,
    uint32_t noteNumber,
    bool onOff,
    int velocity)
{
    if (noteNumber < _minNote)
    {
        _minNote = noteNumber;
    }
    if (noteNumber > _maxNote)
    {
        _maxNote = noteNumber;
    }

    MidiEvent event;
    event.num = noteNumber;
    event.type = MidiEventTypes::M_NOTE;
    event.value = onOff ? velocity : 0;
    event.channel = 0;

    if (_events.find(time) == _events.end())
    {
        _events.insert(std::make_pair(time, std::vector<MidiEvent>{event}));
    }
    else
    {
        _events[time].push_back(event);
    }

    UpdateLength(time);
}

void Region::RemoveEvent(
    std::chrono::milliseconds::rep time,
    uint32_t noteNumber)
{
    if (_events.find(time) == _events.end())
    {
        return;
    }

    auto found = find_if(
        _events[time].begin(),
        _events[time].end(),
        [noteNumber](const MidiEvent &e) {
            return e.num == noteNumber;
        });

    if (found != _events[time].end())
    {
        _events[time].erase(found);
    }
}

void Region::MoveEvent(
    const MidiEvent &e,
    std::chrono::milliseconds::rep from,
    std::chrono::milliseconds::rep to)
{
    if (_events.find(from) == _events.end())
    {
        return;
    }

    auto found = find_if(
        _events[from].begin(),
        _events[from].end(),
        [e](const MidiEvent &ee) {
            return e.channel == ee.channel && e.num == ee.num && e.type == ee.type && e.value == ee.value;
        });

    if (found != _events[from].end())
    {
        _events[from].erase(found);
    }

    _events[to].push_back(e);

    UpdateLength(to);
}

void Region::UpdateLength(
    std::chrono::milliseconds::rep time)
{
    auto requiredLength = time + 4000 - (time % 4000);
    if (_length < requiredLength)
    {
        _length = requiredLength;
    }
}
