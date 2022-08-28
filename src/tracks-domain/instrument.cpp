#include "instrument.h"

Instrument::Instrument() = default;

std::string Instrument::Name() const
{
    return _name;
}

void Instrument::SetName(
    const std::string &name)
{
    _name = name;
}

int Instrument::MidiChannel() const
{
    return _midiChannel;
}

void Instrument::SetMidiChannel(
    int midiChannel)
{
    _midiChannel = midiChannel;
}

VstPlugin *Instrument::Plugin() const
{
    return _plugin;
}

void Instrument::SetPlugin(
    VstPlugin *plugin)
{
    Lock();

    if (_plugin != nullptr)
    {
        _plugin->closeEditor();
        _plugin->cleanup();

        // this wil also call cleanup()
        // TODO : delete this loose end... delete _plugin;
        _plugin = nullptr;
    }

    _plugin = plugin;

    Unlock();
}

void Instrument::Lock()
{
    _mutex.lock();
}

void Instrument::Unlock()
{
    _mutex.unlock();
}
