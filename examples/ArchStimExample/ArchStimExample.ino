#include "ArchStimV3.h"
#include "CommandInterpreter.h"

ArchStimV3 stimDevice;
CommandInterpreter cmdInterpreter(stimDevice);

void setup()
{
  Serial.begin(9600);
  stimDevice.begin();
  stimDevice.beginBLE(cmdInterpreter);

  delay(2000); // Serial connect
  Serial.println("Hello, ARCH Stim.");
  cmdInterpreter.printHelp();
}

void loop()
{
  cmdInterpreter.readSerial();
  stimDevice.runWaveform();
}
