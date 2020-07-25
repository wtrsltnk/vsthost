#include "region.h"

unsigned int Region::GetMinNote() const
{
    return _minNote;
}

unsigned int Region::GetMaxNote() const
{
    return _maxNote;
}

void Region::AddEvent(
    long time,
    unsigned int noteNumber,
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

    _events.insert(std::make_pair(time, std::vector<MidiEvent>{event}));

    auto requiredLength = time + 4000 - (time % 4000);
    if (_length < requiredLength)
    {
        _length = requiredLength;
    }
}