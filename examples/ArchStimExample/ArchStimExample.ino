#include "ArchStimV3.h"
#include "Waveforms/SquareWave.h"

ArchStimV3 stimDevice;

void setup() {
  Serial.begin(9600);
  stimDevice.begin();  // nothing for stimDevice before this
  stimDevice.enableStim();

  delay(2000);  // Serial connect
  stimDevice.beep(1000, 200);
  Serial.println("Hello, ARCH Stim.");
}

void loop() {
  // Check for Serial input to control waveforms
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');

    Serial.println(command);

    if (command == "SQUARE") {
      // Define square wave parameters
      float negVal = -2.0;     // Negative voltage
      float posVal = 2.0;      // Positive voltage
      float frequency = 10.0;  // Frequency in Hz

      // Create a new SquareWave instance with parameters and set as active
      Serial.println("Starting SquareWave...");
      stimDevice.setActiveWaveform(new SquareWave(stimDevice, negVal, posVal, frequency));
    }
    // Add other waveforms as needed
    else if (command == "STOP") {
      // Stop the active waveform
      Serial.println("Stopping waveform.");
      stimDevice.setActiveWaveform(nullptr);
    }
  }

  // Run the active waveform if one is set
  stimDevice.runWaveform();
}
