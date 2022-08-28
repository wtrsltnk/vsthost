#ifndef INSTRUMENT_H
#define INSTRUMENT_H

#include "vstplugin.h"
#include <mutex>
#include <string>

class Instrument
{
public:
    Instrument();
    virtual ~Instrument();

    std::string Name() const;

    void SetName(
        const std::string &name);

    int MidiChannel() const;

    void SetMidiChannel(
        int midiChannel);

    const std::unique_ptr<VstPlugin> &Plugin() const;

    void SetPlugin(
        std::unique_ptr<VstPlugin> plugin);

    void Lock();

    void Unlock();

private:
    std::string _name;
    int _midiChannel = 0;
    std::unique_ptr<VstPlugin> _plugin = nullptr;
    std::mutex _mutex;
};

#endif // INSTRUMENT_H
