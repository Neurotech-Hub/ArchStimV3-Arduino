#include "RampedSineWave.h"

RampedSineWave::RampedSineWave(ArchStimV3 &device, float rampFreq, float weight0, float freq0, int stepSize, int duration)
    : device(device),
      rampFreq(rampFreq),
      weight0(weight0),
      freq0(freq0),
      stepSize(stepSize),
      duration(duration)
{
}

void RampedSineWave::execute()
{
    device.rampedSine(rampFreq, duration, weight0, freq0, stepSize);
}

void RampedSineWave::reset()
{
    // Signal device that waveform timing should be reset
    device.setWaveformResetNeeded();
}