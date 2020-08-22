#ifndef MIDINOTE_H
#define MIDINOTE_H

#include "midievent.h"

#include <chrono>
#include <map>
#include <vector>

class MidiNote
{
public:
    MidiNote();
    MidiNote(
        std::chrono::milliseconds::rep t,
        unsigned int v);

    std::chrono::milliseconds::rep time;
    unsigned int velocity;

public:
    typedef std::vector<MidiNote> Collection;
    typedef std::map<long, Collection> CollectionInTime;
    typedef std::map<unsigned char, CollectionInTime> CollectionInTimeByNote;

    static MidiNote::CollectionInTimeByNote ConvertMidiEventsToMidiNotes(
        const MidiEvent::CollectionInTime &events);
};

#endif // MIDINOTE_H
