#ifndef ARPEGGIATOR_H
#define ARPEGGIATOR_H

#include "midievent.h"

enum class ArpeggiatorModes
{
    PlayOrder,
    Chord,
    Down,
    DownAndUp,
    DownUp,
    Up,
    UpAndDown,
    UpDown,
    Random,
};

enum class ArpeggiatorRates
{
    QuarterNote = 4,
    EightNote = 8,
    SixteenthNote = 16,
    ThirtySecondNote = 32,
};

class ArpeggiatorNote
{
public:
    uint32_t Velocity = 127;

    uint32_t Note = 0;
};

class Arpeggiator
{
public:
    Arpeggiator();

    ArpeggiatorModes Mode = ArpeggiatorModes::PlayOrder;

    ArpeggiatorRates Rate = ArpeggiatorRates::QuarterNote;

    std::vector<ArpeggiatorNote> Notes;

    float Length = 1.0f;

    MidiEvent::CollectionInTime GetMidiEvents() const;
};

#endif // ARPEGGIATOR_H
