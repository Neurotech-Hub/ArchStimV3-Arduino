// SineWave.h
#ifndef SINEWAVE_H
#define SINEWAVE_H

#include "../Waveforms/Waveform.h" // Base waveform class
#include "../ArchStimV3.h"         // Access to ArchStimV3 functions

class SineWave : public Waveform
{
public:
    SineWave(ArchStimV3 &device, int amplitude, float frequency);
    void execute() override;
    void reset() override; // Reset waveform timing

private:
    ArchStimV3 &device;
    int amplitude;
    float frequency;
    unsigned long lastUpdateTime;
};

#endif