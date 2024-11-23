#include "ArchStimV3.h"
#include "CommandInterpreter.h"

ArchStimV3 stimDevice;
CommandInterpreter cmdInterpreter(stimDevice);

void setup()
{
  Serial.begin(9600);
  stimDevice.begin();
  stimDevice.enableStim();
  stimDevice.beginBLE(cmdInterpreter);

  delay(2000); // Serial connect
  stimDevice.beep(1000, 200);
  Serial.println("Hello, ARCH Stim.");
  cmdInterpreter.printHelp();
}

void loop()
{
  if (Serial.available())
  {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (!cmdInterpreter.processCommand(command))
    {
      Serial.println("Command failed. Type HELP for usage.");
    }
  }

  stimDevice.runWaveform();
}
