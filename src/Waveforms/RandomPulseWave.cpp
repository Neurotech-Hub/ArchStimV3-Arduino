#include "RandomPulseWave.h"

RandomPulseWave::RandomPulseWave(ArchStimV3 &device, float *ampArray, int arrSize)
    : device(device), arrSize(arrSize), inZeroState(true), lastTransitionTime(0)
{

    // Allocate and copy array to prevent dangling pointers
    this->ampArray = new float[arrSize];
    for (int i = 0; i < arrSize; i++)
    {
        this->ampArray[i] = ampArray[i];
    }

    // Initial setup
    transitionToZeroState();
}

RandomPulseWave::~RandomPulseWave()
{
    delete[] ampArray;
}

void RandomPulseWave::transitionToZeroState()
{
    inZeroState = true;
    currentAmplitude = 0;
    currentDuration = 1000 + random(501); // 1000ms + random(0-500)ms
    device.dac.setAllVoltages(currentAmplitude);
}

void RandomPulseWave::transitionToActiveState()
{
    inZeroState = false;
    currentAmplitude = ampArray[random(arrSize)];  // Random amplitude from array
    currentDuration = (random(2) == 0) ? 25 : 100; // Randomly choose 25ms or 100ms
    device.dac.setAllVoltages(currentAmplitude);
}

void RandomPulseWave::execute()
{
    unsigned long currentTime = millis();

    if (currentTime - lastTransitionTime >= currentDuration)
    {
        lastTransitionTime = currentTime;

        if (inZeroState)
        {
            transitionToActiveState();
        }
        else
        {
            transitionToZeroState();
        }
    }
}