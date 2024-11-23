#include "PulseWave.h"

PulseWave::PulseWave(ArchStimV3 &device, float *ampArray, int *timeArray, int arrSize)
    : device(device), arrSize(arrSize), currentIndex(0), lastTransitionTime(0)
{
    // Determine if we're in single duration mode
    singleDurationMode = (timeArray[1] == 0); // Assuming second element is 0 for single mode

    // Allocate and copy arrays to prevent dangling pointers
    this->ampArray = new float[arrSize];
    this->timeArray = new int[singleDurationMode ? 1 : arrSize];

    // Copy amplitude array
    for (int i = 0; i < arrSize; i++)
    {
        this->ampArray[i] = ampArray[i];
    }

    // Copy time array (either just first element or all elements)
    if (singleDurationMode)
    {
        this->timeArray[0] = timeArray[0];
    }
    else
    {
        for (int i = 0; i < arrSize; i++)
        {
            this->timeArray[i] = timeArray[i];
        }
    }
}

PulseWave::~PulseWave()
{
    delete[] ampArray;
    delete[] timeArray;
}

void PulseWave::execute()
{
    unsigned long currentTime = millis();

    // Get the current duration (either single value or from array)
    int currentDuration = singleDurationMode ? timeArray[0] : timeArray[currentIndex];

    // Check if it's time to transition to the next value
    if (currentTime - lastTransitionTime >= currentDuration)
    {
        // Move to next index, wrapping around to 0 if we reach the end
        currentIndex = (currentIndex + 1) % arrSize;
        lastTransitionTime = currentTime;

        // Set the new voltage
        device.dac.setAllVoltages(ampArray[currentIndex]);
    }
}