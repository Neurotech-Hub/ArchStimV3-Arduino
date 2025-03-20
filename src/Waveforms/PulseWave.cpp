#include "PulseWave.h"

PulseWave::PulseWave(ArchStimV3 &device, int *ampArray, int *timeArray, int arrSize)
    : device(device), arrSize(arrSize)
{
    // Allocate and copy arrays to prevent dangling pointers
    this->ampArray = new int[arrSize];
    this->timeArray = new int[arrSize];

    // Copy arrays
    for (int i = 0; i < arrSize; i++)
    {
        this->ampArray[i] = ampArray[i];
        this->timeArray[i] = timeArray[i];
    }
}

PulseWave::~PulseWave()
{
    delete[] ampArray;
    delete[] timeArray;
}

void PulseWave::execute()
{
    device.pulse(ampArray, timeArray, arrSize);
}

void PulseWave::reset()
{
    // Signal device that waveform timing should be reset
    device.setWaveformResetNeeded();
}