#include "SumOfSinesWave.h"
#include <Arduino.h>

SumOfSinesWave::SumOfSinesWave(ArchStimV3 &device, float weight0, float freq0, float weight1, float freq1, int stepSize, int duration)
    : device(device),
      weight0(weight0),
      freq0(freq0),
      weight1(weight1),
      freq1(freq1),
      stepSize(stepSize),
      duration(duration),
      startTime(millis()),
      lastStepTime(0)
{
}

void SumOfSinesWave::execute()
{
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - startTime;

    // Check if the waveform duration has elapsed
    if (duration > 0 && elapsedTime >= (unsigned long)duration)
    {
        device.setActiveWaveform(nullptr);
        return;
    }

    // Only update at specified step intervals
    if (currentTime - lastStepTime >= stepSize)
    {
        // Calculate time in seconds for sine functions
        float t = elapsedTime / 1000.0f;

        // Calculate the sum of sines
        float value = weight0 * sin(2 * PI * freq0 * t) +
                      weight1 * sin(2 * PI * freq1 * t);

        // Update the output
        device.dac.setAllVoltages(value);
        lastStepTime = currentTime;
    }
}