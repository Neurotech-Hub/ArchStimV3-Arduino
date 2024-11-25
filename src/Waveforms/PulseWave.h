#ifndef PULSEWAVE_H
#define PULSEWAVE_H

#include "../Waveforms/Waveform.h"
#include "../ArchStimV3.h"

class PulseWave : public Waveform
{
public:
    PulseWave(ArchStimV3 &device, int *ampArray, int *timeArray, int arrSize);
    ~PulseWave();
    void execute() override;

private:
    ArchStimV3 &device;
    int *ampArray;
    int *timeArray;
    int arrSize;
};

#endif