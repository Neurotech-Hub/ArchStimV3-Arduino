// SquareWave.h
#ifndef SQUAREWAVE_H
#define SQUAREWAVE_H

#include "../Waveforms/Waveform.h" // Base waveform class
#include "../ArchStimV3.h"         // Access to ArchStimV3 functions

class SquareWave : public Waveform
{
public:
    SquareWave(ArchStimV3 &device, int negVal, int posVal, float frequency);
    void execute() override;
    void reset() override; // Reset waveform timing

private:
    ArchStimV3 &device;
    int negVal;
    int posVal;
    float frequency;
};

#endif
