// SquareWave.cpp
#include "SquareWave.h"

// Constructor initializes the device and parameters
SquareWave::SquareWave(ArchStimV3& device, float negVal, float posVal, float frequency)
    : device(device), negVal(negVal), posVal(posVal), frequency(frequency) {}

void SquareWave::execute() {
    // The square waveform generation logic, using negVal, posVal, and frequency
    device.square(negVal, posVal, frequency);
}
