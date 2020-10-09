#include "instrument.h"

Instrument::Instrument()
{
}

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
    _plugin = plugin;
}
