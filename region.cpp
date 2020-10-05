#include "region.h"

#include <iostream>

uint32_t Region::GetMinNote() const
{
    return _minNote;
}

uint32_t Region::GetMaxNote() const
{
    return _maxNote;
}

void Region::AddEvent(
    long time,
    uint32_t noteNumber,
    bool onOff,
    int velocity)
{
    std::cout << "time : " << time << std::endl
              << "noteNumber : " << noteNumber << std::endl
              << "onOff : " << onOff << std::endl
              << "velocity : " << velocity << std::endl;

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

    auto requiredLength = time + 4000 - (time % 4000);
    if (_length < requiredLength)
    {
        _length = requiredLength;
    }
}

void Region::RemoveEvent(
    long time,
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
    long from,
    long to)
{
    std::cout << "moving from " << from << " to " << to << "\n";
    if (_events.find(from) == _events.end())
    {
        std::cout << "from not found\n";
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
    else
    {
        std::cout << "not found\n";
    }
    _events[to].push_back(e);
}
