#include "SumOfSinesWave.h"

SumOfSinesWave::SumOfSinesWave(ArchStimV3 &device, float weight0, float freq0, float weight1, float freq1, int stepSize, int duration)
    : device(device),
      weight0(weight0),
      freq0(freq0),
      weight1(weight1),
      freq1(freq1),
      stepSize(stepSize),
      duration(duration)
{
}

void SumOfSinesWave::execute()
{
    device.sumOfSines(weight0, freq0, weight1, freq1, stepSize, duration);
}