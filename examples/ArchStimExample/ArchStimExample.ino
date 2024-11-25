#include "ArchStimV3.h"
#include "Waveforms/SquareWave.h"

ArchStimV3 stimDevice;

void setup()
{
  Serial.begin(9600);
  stimDevice.begin(); // nothing for stimDevice before this
  stimDevice.enableStim();

  delay(1000); // Serial connect
  stimDevice.beep(1000, 200);
  Serial.println("Hello, ARCH Stim.");
}

void loop()
{
  // Check for Serial input to control waveforms
  if (Serial.available())
  {
    String command = Serial.readStringUntil('\n');

    Serial.println(command);

    if (command == "SQUARE")
    {
      // Define square wave parameters
      float negVal = -2.0;    // Negative voltage
      float posVal = 2.0;     // Positive voltage
      float frequency = 10.0; // Frequency in Hz

      // Create a new SquareWave instance with parameters and set as active
      Serial.println("Starting SquareWave...");
      stimDevice.setActiveWaveform(new SquareWave(stimDevice, negVal, posVal, frequency));
    }
    // Add other waveforms as needed
    else if (command == "STOP")
    {
      // Stop the active waveform
      Serial.println("Stopping waveform.");
      stimDevice.setActiveWaveform(nullptr);
    }
    // Add ADC command handling
    else if (command.startsWith("ADC"))
    {
      // Parse CS delay from command (e.g., "ADC,100")
      int commaIndex = command.indexOf(',');
      if (commaIndex != -1)
      {
        uint16_t csDelay = command.substring(commaIndex + 1).toInt();
        stimDevice.setCSDelay(csDelay);
        for (int i = 0; i < 20; i++)
        {
          Serial.println(stimDevice.getMilliVolts(0)); // Using channel 0
        }
      }
    }
    else if (command.startsWith("SET:"))
    {
      String voltageStr = command.substring(4); // Get everything after "SET:"
      float voltage = voltageStr.toFloat();
      stimDevice.setVoltage(voltage);
      Serial.print("Setting voltage to: ");
      Serial.print(voltage);
      Serial.println("V");
    }
    else if (command == "ON")
    {
      Serial.println("Activating isolated components...");
      stimDevice.activateIsolated();
      Serial.println("Isolated components activated.");
    }
    else if (command == "OFF")
    {
      Serial.println("Deactivating isolated components...");
      stimDevice.deactivateIsolated();
      Serial.println("Isolated components deactivated.");
    }
  }

  // Run the active waveform if one is set
  stimDevice.runWaveform();
}
