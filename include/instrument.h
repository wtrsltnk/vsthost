#ifndef INSTRUMENT_H
#define INSTRUMENT_H

#include "vstplugin.h"
#include <mutex>
#include <string>

#define MAX_EFFECT_PLUGINS 4

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

    const std::shared_ptr<VstPlugin> &InstrumentPlugin() const;

    void SetInstrumentPlugin(
        std::shared_ptr<VstPlugin> plugin);

    const std::shared_ptr<VstPlugin> EffectPlugin(
        int index) const;

    void SetEffectPlugin(
        int index,
        std::shared_ptr<VstPlugin> plugin);

    void Lock();

    void Unlock();

private:
    std::string _name;
    int _midiChannel = 0;
    std::shared_ptr<VstPlugin> _plugin = nullptr;
    std::shared_ptr<VstPlugin> _effectPlugins[MAX_EFFECT_PLUGINS] = {nullptr};
    std::mutex _mutex;
};

#endif // INSTRUMENT_H
