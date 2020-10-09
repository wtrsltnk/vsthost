#ifndef ITRACK_H
#define ITRACK_H

#include "region.h"
#include <map>
#include <string>
#include <chrono>

class Instrument;

class ITrack
{
public:
    virtual ~ITrack() = default;

    virtual const std::string &GetName() = 0;
    virtual void SetName(
        std::string const &name) = 0;

    virtual Instrument *GetInstrument() = 0;
    virtual void SetInstrument(
        Instrument *instrument) = 0;

    virtual bool IsMuted() = 0;
    virtual void Mute() = 0;
    virtual void Unmute() = 0;
    virtual void ToggleMuted() = 0;

    virtual bool IsReadyForRecoding() = 0;
    virtual void SetReadyForRecording(
        bool ready) = 0;
    virtual void ToggleReadyForRecording() = 0;

    virtual float *GetColor() = 0;
    virtual void SetColor(
        float color[]) = 0;
    virtual void SetColor(
        float r,
        float g,
        float b,
        float a = 1.0f) = 0;

    virtual std::map<std::chrono::milliseconds::rep, Region> const &Regions() const = 0;

    virtual Region &GetRegion(
        std::chrono::milliseconds::rep at) = 0;

    virtual void StartRecording() = 0;

    virtual long StartNewRegion(
        std::chrono::milliseconds::rep start) = 0;

    virtual void RecordMidiEvent(
        std::chrono::milliseconds::rep time,
        int noteNumber,
        bool onOff,
        int velocity) = 0;

    virtual void AddRegion(
        std::chrono::milliseconds::rep startAt,
        Region const &region) = 0;

    virtual void RemoveRegion(
        std::chrono::milliseconds::rep startAt) = 0;
};

#endif // ITRACK_H
