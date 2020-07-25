#ifndef MIDICONTROLLERS_H
#define MIDICONTROLLERS_H

enum MidiControllers
{
    C_bankselectmsb = 0,
    C_pitchwheel = 1000,
    C_NULL = 1001,
    C_expression = 11,
    C_panning = 10,
    C_bankselectlsb = 32,
    C_filtercutoff = 74,
    C_filterq = 71,
    C_bandwidth = 75,
    C_modwheel = 1,
    C_fmamp = 76,
    C_volume = 7,
    C_sustain = 64,
    C_allnotesoff = 123,
    C_allsoundsoff = 120,
    C_resetallcontrollers = 121,
    C_portamento = 65,
    C_resonance_center = 77,
    C_resonance_bandwidth = 78,

    C_dataentryhi = 0x06,
    C_dataentrylo = 0x26,
    C_nrpnhi = 99,
    C_nrpnlo = 98
};

#endif // MIDICONTROLLERS_H
