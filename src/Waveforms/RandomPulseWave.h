#ifndef RANDOMPULSEWAVE_H
#define RANDOMPULSEWAVE_H

#include "../Waveforms/Waveform.h"
#include "../ArchStimV3.h"

class RandomPulseWave : public Waveform
{
public:
    RandomPulseWave(ArchStimV3 &device, int *ampArray, int arrSize);
    ~RandomPulseWave();
    void execute() override;
    void reset() override; // Reset waveform timing

private:
    ArchStimV3 &device;
    int *ampArray;
    int arrSize;
};

#endif