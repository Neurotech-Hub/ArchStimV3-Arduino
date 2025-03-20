#ifndef RAMPEDSINEWAVE_H
#define RAMPEDSINEWAVE_H

#include "../Waveforms/Waveform.h"
#include "../ArchStimV3.h"

class RampedSineWave : public Waveform
{
public:
    RampedSineWave(ArchStimV3 &device, float rampFreq, float weight0, float freq0, int stepSize, int duration);
    void execute() override;
    void reset() override; // Reset waveform timing

private:
    ArchStimV3 &device;
    float rampFreq;
    float weight0;
    float freq0;
    int stepSize;
    int duration;
};

#endif