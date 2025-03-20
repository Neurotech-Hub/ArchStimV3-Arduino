// SineWave.cpp
#include "SineWave.h"

SineWave::SineWave(ArchStimV3 &device, int amplitude, float frequency)
    : device(device), amplitude(amplitude), frequency(frequency), lastUpdateTime(0) {}

void SineWave::execute()
{
    device.sine(amplitude, frequency);
}

void SineWave::reset()
{
    // Signal device that waveform timing should be reset
    device.setWaveformResetNeeded();
}