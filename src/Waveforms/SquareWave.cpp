// SquareWave.cpp
#include "SquareWave.h"

// Constructor initializes the device and parameters (values in volts)
SquareWave::SquareWave(ArchStimV3 &device, int negVal, int posVal, float frequency)
    : device(device), negVal(negVal), posVal(posVal), frequency(frequency) {}

void SquareWave::execute()
{
    // Pass voltage values to device.square(), which handles conversion to microamps
    device.square(negVal, posVal, frequency);
}

void SquareWave::reset()
{
    // Signal device that waveform timing should be reset
    device.setWaveformResetNeeded();
}
