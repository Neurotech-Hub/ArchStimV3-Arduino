#ifndef SUMOFSINESWAVE_H
#define SUMOFSINESSWAVE_H

#include "../Waveforms/Waveform.h"
#include "../ArchStimV3.h"

class SumOfSinesWave : public Waveform
{
public:
    SumOfSinesWave(ArchStimV3 &device, float weight0, float freq0, float weight1, float freq1, int stepSize, int duration);
    void execute() override;
    void reset() override; // Reset waveform timing

private:
    ArchStimV3 &device;
    float weight0;
    float freq0;
    float weight1;
    float freq1;
    int stepSize;
    int duration;
};

#endif