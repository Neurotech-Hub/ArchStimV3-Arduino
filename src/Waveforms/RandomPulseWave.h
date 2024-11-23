#ifndef RANDOMPULSEWAVE_H
#define RANDOMPULSEWAVE_H

#include "../Waveforms/Waveform.h"
#include "../ArchStimV3.h"

class RandomPulseWave : public Waveform
{
public:
    RandomPulseWave(ArchStimV3 &device, float *ampArray, int arrSize);
    ~RandomPulseWave();
    void execute() override;

private:
    ArchStimV3 &device;
    float *ampArray;
    int arrSize;
    bool inZeroState;
    unsigned long lastTransitionTime;
    unsigned long currentDuration;
    float currentAmplitude;

    void transitionToZeroState();
    void transitionToActiveState();
};

#endif