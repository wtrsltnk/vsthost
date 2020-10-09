#ifndef REGION_H
#define REGION_H

#include "midievent.h"

#include <algorithm>
#include <chrono>
#include <limits>
#include <map>
#include <numeric>
#include <string>
#include <vector>

class Region
{
public:
    const std::string &GetName() const;

    void SetName(
        const std::string &name);

    long Length() const;

    void SetLength(
        long length);

    const std::map<long, std::vector<MidiEvent>> &Events() const;

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

    void MoveEvent(
        const MidiEvent &e,
        long from,
        long to);

private:
    std::string _name = "region";
    long _length = 16000;
    std::map<long, std::vector<MidiEvent>> _events; // the long key of the map is the relaive start of the event from the beginning of the region
    std::map<long, std::vector<MidiEvent>> _selection;

    uint32_t _minNote = std::numeric_limits<uint32_t>::max();
    uint32_t _maxNote = 0;

    void UpdateLength(
        long time);
};

#endif // REGION_H
