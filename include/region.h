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

    std::chrono::milliseconds::rep Length() const;

    void SetLength(
        std::chrono::milliseconds::rep length);

    const MidiEvent::CollectionInTime &Events() const;

    uint32_t GetMinNote() const;

    uint32_t GetMaxNote() const;

    void AddEvent(
        std::chrono::milliseconds::rep time,
        uint32_t noteNumber,
        bool onOff,
        int velocity);

    void RemoveEvent(
        std::chrono::milliseconds::rep time,
        uint32_t noteNumber);

    void MoveEvent(
        const MidiEvent &e,
        std::chrono::milliseconds::rep from,
        std::chrono::milliseconds::rep to);

private:
    std::string _name = "region";
    std::chrono::milliseconds::rep _length = 16000;
    MidiEvent::CollectionInTime _events; // the long key of the map is the relaive start of the event from the beginning of the region
    MidiEvent::CollectionInTime _selection;

    uint32_t _minNote = (std::numeric_limits<uint32_t>::max)();
    uint32_t _maxNote = 0;

    void UpdateLength(
        std::chrono::milliseconds::rep time);
};

#endif // REGION_H
