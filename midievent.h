#ifndef MIDIEVENT_H
#define MIDIEVENT_H

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
    unsigned int channel; // the midi channel for the event
    MidiEventTypes type;  // type=1 for note, type=2 for controller
    unsigned int num;     // note, controller or program number
    unsigned int value;   // velocity or controller value
};

#endif // MIDIEVENT_H
