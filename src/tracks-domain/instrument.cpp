#include "instrument.h"

Instrument::Instrument() = default;

Instrument::~Instrument() = default;

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

const std::shared_ptr<VstPlugin> &Instrument::InstrumentPlugin() const
{
    return _plugin;
}

void Instrument::SetInstrumentPlugin(
    std::shared_ptr<VstPlugin> plugin)
{
    Lock();

    if (_plugin != nullptr)
    {
        _plugin->closeEditor();
        _plugin->cleanup();
    }

    _plugin = plugin;

    Unlock();
}

const std::shared_ptr<VstPlugin> Instrument::EffectPlugin(
    int index) const
{
    if (index < 0)
    {
        return nullptr;
    }

    if (index >= MAX_EFFECT_PLUGINS)
    {
        return nullptr;
    }

    return _effectPlugins[index];
}

void Instrument::SetEffectPlugin(
    int index,
    std::shared_ptr<VstPlugin> plugin)
{
    if (index < 0)
    {
        return;
    }

    if (index >= MAX_EFFECT_PLUGINS)
    {
        return;
    }

    Lock();

    if (_effectPlugins[index] != nullptr)
    {
        _effectPlugins[index]->closeEditor();
        _effectPlugins[index]->cleanup();
    }

    _effectPlugins[index] = plugin;

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
