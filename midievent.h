#ifndef MIDIEVENT_H
#define MIDIEVENT_H

#include <map>
#include <vector>

struct MidiNoteState
{
    explicit MidiNoteState(int midiNote);
    int midiNote;
    bool status;
};

enum class MidiEventTypes
{
    M_NOTE = 1,       // note
    M_CONTROLLER = 2, // controller
    M_PGMCHANGE = 3,  // program change
    M_PRESSURE = 4    // polyphonic aftertouch
};

struct MidiEvent
{
    MidiEvent();
    uint32_t channel; // the midi channel for the event
    MidiEventTypes type;  // type=1 for note, type=2 for controller
    uint32_t num;     // note, controller or program number
    uint32_t value;   // velocity or controller value

    typedef std::vector<MidiEvent> Collection;
    typedef std::map<long, Collection> CollectionInTime;
};

#endif // MIDIEVENT_H
