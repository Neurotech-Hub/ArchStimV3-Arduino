#include "RampedSineWave.h"
#include <Arduino.h>

RampedSineWave::RampedSineWave(ArchStimV3 &device, float rampFreq, float weight0, float freq0, int stepSize, int duration)
    : device(device),
      rampFreq(rampFreq),
      weight0(weight0),
      freq0(freq0),
      stepSize(stepSize),
      duration(duration),
      startTime(millis()),
      lastStepTime(0)
{
}

void RampedSineWave::execute()
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

        // Calculate the ramped sine wave
        float envelope = abs(sin(PI * rampFreq * t));
        float value = weight0 * envelope * sin(2 * PI * freq0 * t);

        // Update the output
        device.dac.setAllVoltages(value);
        lastStepTime = currentTime;
    }
}