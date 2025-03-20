#include "RandomPulseWave.h"

RandomPulseWave::RandomPulseWave(ArchStimV3 &device, int *ampArray, int arrSize)
    : device(device), arrSize(arrSize)
{
    // Allocate and copy array to prevent dangling pointers
    this->ampArray = new int[arrSize];

    // Copy array
    for (int i = 0; i < arrSize; i++)
    {
        this->ampArray[i] = ampArray[i];
    }
}

RandomPulseWave::~RandomPulseWave()
{
    delete[] ampArray;
}

void RandomPulseWave::execute()
{
    device.randPulse(ampArray, arrSize);
}

void RandomPulseWave::reset()
{
    // Signal device that waveform timing should be reset
    device.setWaveformResetNeeded();
}