#ifndef REGION_H
#define REGION_H

#include "midievent.h"

#include <algorithm>
#include <chrono>
#include <limits>
#include <map>
#include <numeric>
#include <vector>

class Region
{
    unsigned int _minNote = std::numeric_limits<unsigned int>::max();
    unsigned int _maxNote = 0;

public:
    long _length;
    std::map<long, std::vector<MidiEvent>> _events; // the int key of the map is the relaive start of the note from the beginning of the region
    std::map<long, std::vector<MidiEvent>> _selection;

    unsigned int GetMinNote() const;

    unsigned int GetMaxNote() const;

    void AddEvent(
        long time,
        unsigned int noteNumber,
        bool onOff,
        int velocity);
};

#endif // REGION_H
