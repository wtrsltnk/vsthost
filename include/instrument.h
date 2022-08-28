#ifndef INSTRUMENT_H
#define INSTRUMENT_H

#include "vstplugin.h"
#include <mutex>
#include <string>

class Instrument
{
public:
    Instrument();

    std::string Name() const;

    void SetName(
        const std::string &name);

    int MidiChannel() const;

    void SetMidiChannel(
        int midiChannel);

    VstPlugin *Plugin() const;

    void SetPlugin(
        VstPlugin *plugin);

    void Lock();

    void Unlock();

private:
    std::string _name;
    int _midiChannel = 0;
    VstPlugin *_plugin = nullptr;
    std::mutex _mutex;
};

#endif // INSTRUMENT_H
