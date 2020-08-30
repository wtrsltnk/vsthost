#ifndef MIDINOTE_H
#define MIDINOTE_H

#include "midievent.h"

#include <chrono>
#include <map>
#include <vector>

const int Note_C_OffsetFromC = 0;
const int Note_CSharp_OffsetFromC = 1;
const int Note_D_OffsetFromC = 2;
const int Note_DSharp_OffsetFromC = 3;
const int Note_E_OffsetFromC = 4;
const int Note_F_OffsetFromC = 5;
const int Note_FSharp_OffsetFromC = 6;
const int Note_G_OffsetFromC = 7;
const int Note_GSharp_OffsetFromC = 8;
const int Note_A_OffsetFromC = 9;
const int Note_ASharp_OffsetFromC = 10;
const int Note_B_OffsetFromC = 11;
const int firstKeyNoteNumber = 24;

class MidiNote
{
public:
    MidiNote();
    MidiNote(
        std::chrono::milliseconds::rep t,
        unsigned int v);

    std::chrono::milliseconds::rep length;
    unsigned int velocity;

public:
    typedef std::vector<MidiNote> Collection;
    typedef std::map<long, Collection> CollectionInTime;
    typedef std::map<unsigned char, CollectionInTime> CollectionInTimeByNote;

    static MidiNote::CollectionInTimeByNote ConvertMidiEventsToMidiNotes(
        const MidiEvent::CollectionInTime &events);
};

#endif // MIDINOTE_H
