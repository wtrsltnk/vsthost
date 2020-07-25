#ifndef INSTRUMENT_H
#define INSTRUMENT_H

#include "vstplugin.h"
#include <string>

class Instrument
{
public:
    Instrument();
    std::string _name;
    int _midiChannel;
    VstPlugin *_plugin = nullptr;
};

#endif // INSTRUMENT_H
