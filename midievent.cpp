#include "midievent.h"

MidiNoteState::MidiNoteState(
    int midiNote)
    : midiNote(midiNote),
      status(false)
{}

MidiEvent::MidiEvent()
    : channel(0),
      type(MidiEventTypes::M_NOTE),
      num(0),
      value(0)
{}
