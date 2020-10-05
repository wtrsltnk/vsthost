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
    uint32_t _minNote = std::numeric_limits<uint32_t>::max();
    uint32_t _maxNote = 0;

public:
    long _length = 16000;
    std::map<long, std::vector<MidiEvent>> _events; // the long key of the map is the relaive start of the event from the beginning of the region
    std::map<long, std::vector<MidiEvent>> _selection;

    uint32_t GetMinNote() const;

    uint32_t GetMaxNote() const;

    void AddEvent(
        long time,
        uint32_t noteNumber,
        bool onOff,
        int velocity);

    void RemoveEvent(
        long time,
        uint32_t noteNumber);
};

#endif // REGION_H
