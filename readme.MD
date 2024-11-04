# ArchStimV3 Library

`ArchStimV3` is an Arduino library designed to control and generate complex waveforms with an ADC (ADS1118) and a DAC (AD57X4R). This library allows for the generation of various waveforms and provides utility functions for managing and controlling isolated components.

## Features

- **Initialization** of ADC, DAC, SPI, I2C, and pins
- **Waveform generation**: Square wave, pulse sequences, random pulses, time series, and more
- **Active waveform management** with support for dynamically switching waveforms
- **Utility functions** for enabling/disabling isolated components and checking impedance

## Requirements

- **Arduino IDE** or compatible development environment
- **ADS1118** ADC library (for analog-to-digital conversion)
- **AD57X4R** DAC library (for digital-to-analog conversion)
- **SD.h, SPI.h, Wire.h** libraries (included with the Arduino IDE)

## Installation

1. Clone or download the `ArchStimV3` library into your Arduino `libraries` folder.
2. Make sure the required dependencies (ADS1118 and AD57X4R) are installed in your Arduino IDE.
3. Restart the Arduino IDE if it was open during installation.

## Pin Configuration

| Pin Name      | Pin Number | Description                   |
|---------------|------------|-------------------------------|
| USB_SENSE     | 1          | USB power sense input        |
| USER_IN       | 2          | User input with pull-up      |
| LED_B         | 5          | Blue LED                     |
| EXT_OUTPUT    | 9          | External output control      |
| BUZZ          | 10         | Buzzer output                |
| EXT_INPUT     | 11         | External input               |
| ADC_CS        | 12         | ADC chip select              |
| DAC_CS        | 13         | DAC chip select              |
| LED_R         | 45         | Red LED                      |
| LED_STIM      | 46         | Stimulation LED              |
| MISO          | 37         | SPI MISO                     |
| SCK           | 36         | SPI SCK                      |
| MOSI          | 35         | SPI MOSI                     |
| SD_CS         | 34         | SD card chip select          |
| FUEL_ALERT    | 42         | Fuel gauge alert input       |
| DISABLE       | 41         | Disable output               |
| DRIVE_EN      | 40         | Drive enable for isolation   |
| SDA_PIN       | 3          | I2C SDA                      |
| SCL_PIN       | 4          | I2C SCL                      |

## Usage

### Basic Initialization

```cpp
#include <ArchStimV3.h>

ArchStimV3 stimDevice;

void setup() {
    Serial.begin(9600);
    stimDevice.begin();
    stimDevice.activateIsolated();
    stimDevice.enableStim();
}
```

### Starting a Square Waveform

```cpp
#include "Waveforms/SquareWave.h"

// Start a 10 Hz square wave with ±2V amplitude
float negVal = -2.0;
float posVal = 2.0;
float frequency = 10.0;
stimDevice.setActiveWaveform(new SquareWave(stimDevice, negVal, posVal, frequency));
```

### Stopping the Active Waveform

```cpp
stimDevice.setActiveWaveform(nullptr);  // Stops the active waveform
```

### Running the Active Waveform

In the main `loop()`, continuously call `runWaveform()` to ensure the active waveform executes:

```cpp
void loop() {
    stimDevice.runWaveform();
}
```

## API Reference

### Initialization Functions

- **`void begin()`**: Initializes all necessary peripherals (pins, SPI, I2C, ADC, DAC).
- **`void initPins()`**: Configures I/O pins with default states.
- **`void initSPI()`**: Sets up SPI communication.
- **`void initI2C()`**: Sets up I2C communication.
- **`void initADC()`**: Configures the ADS1118 ADC.
- **`void initDAC()`**: Configures the AD57X4R DAC.

### Waveform Generation Functions

- **`void square(float negVal, float posVal, float frequency)`**: Generates a square wave with specified negative and positive values and frequency.
- **`void pulse(float ampArray[], int timeArray[], int arrSize)`**: Generates a pulse sequence based on amplitude and timing arrays.
- **`void randPulse(float ampArray[], int arrSize)`**: Creates random pulses with specified amplitudes.
- **`void readTimeSeries(float ampArray[], int arrSize, int stepSize)`**: Reads a time series based on the provided amplitude array and step size.
- **`void sumOfSines(int stepSize, float weight0, float freq0, float weight1, float freq1, int duration)`**: Generates a waveform by summing two sine waves.
- **`void rampedSine(float rampFreq, float duration, float weight0, float freq0, int stepSize)`**: Creates a ramped sine wave.

### Utility Functions

- **`float zCheck()`**: Returns the current impedance.
- **`void printCurrent()`**: Prints the current measurement.
- **`void activateIsolated()`**: Enables isolated components.
- **`void deactivateIsolated()`**: Disables isolated components.
- **`void enableStim()`**: Enables stimulation.
- **`void disableStim()`**: Disables stimulation.
- **`void beep(int frequency, int duration)`**: Activates the buzzer with the specified frequency and duration.

### Waveform Management

- **`void setActiveWaveform(Waveform* waveform)`**: Sets the current active waveform, replacing any existing waveform.
- **`void runWaveform()`**: Executes the active waveform, if one is set.

## License

This library is provided under the MIT License. See `LICENSE` for details.

---

## Notes

### Code to be ported...

void ArchStimV3::square(float negVal, float posVal, float frequency) {
    static bool highState = false;
    static unsigned long lastToggleTime = 0;
    unsigned long interval = 1000000 / (2 * frequency);  // Half-period in microseconds

    unsigned long currentTime = micros();
    if (currentTime - lastToggleTime >= interval) {
        lastToggleTime = currentTime;
        highState = !highState;
        
        // Set DAC output based on state
        dac.setAllVoltages(highState ? posVal : negVal);
    }
}

void ArchStimV3::pulse(float ampArray[], int timeArray[], int arrSize) {
    // long currentTime = millis();  // Initialize time value
    // Z = zCheck();                 // Get head impedance (adjust as needed for your application)

    // while (true) {
    //     for (int i = 0; i < arrSize; i++) {
    //         // Set DAC output directly using amplitude from ampArray, scaled as needed
    //         float amplitude = ampArray[i];
    //         float dacValue = min(DAC_MAX, max(DAC_MIN, amplitude));

    //         // Set DAC to calculated output level
    //         dac.setVoltage(dacValue);

    //         // Track timing for each pulse
    //         currentTime = millis();
    //         long targetTime = currentTime + timeArray[i];
    //         while (millis() < targetTime) {
    //             printCurrent();  // Keep updating or logging the current during the pulse period
    //         }
    //         // Check for "end" command to stop
    //     }
    // }
}

void ArchStimV3::randPulse(float ampArray[], int arrSize) {
    // Z = zCheck();  // Get impedance, if required

    // while (true) {
    //     // Randomly select an amplitude from the array
    //     int randomIndex = random(0, arrSize);  // Randomly pick an index in the range
    //     float amplitude = ampArray[randomIndex];

    //     // Clamp the amplitude value between DAC_MIN and DAC_MAX
    //     float dacValue = min(DAC_MAX, max(DAC_MIN, amplitude));

    //     // Set DAC output
    //     dac.setVoltage(dacValue);

    //     // Set random duration for pulse between 50 ms and 500 ms
    //     int pulseDuration = random(50, 500);
    //     long startTime = millis();
    //     while (millis() - startTime < pulseDuration) {
    //         printCurrent();  // Continuously monitor or log during the pulse
    //     }

    //     // Check for "end" command to stop
    // }
}

void ArchStimV3::readTimeSeries(float ampArray[], int arrSize, int stepSize) {
    // for (int i = 0; i < arrSize; i++) {
    //     // Clamp the amplitude value between DAC_MIN and DAC_MAX
    //     float dacValue = min(DAC_MAX, max(DAC_MIN, ampArray[i]));

    //     // Set DAC output
    //     dac.setVoltage(dacValue);

    //     // Wait for the specified step size duration
    //     delay(stepSize);
    // }
}

void ArchStimV3::sumOfSines(int stepSize, float weight0, float freq0, float weight1, float freq1, int duration) {
    // unsigned long startTime = millis();
    // unsigned long currentTime, nextStepTime;
    // float currentSec;
    // float amplitude;
    
    // Z = zCheck();  // Find head impedance

    // while (millis() - startTime < duration) {
    //     currentTime = millis();
    //     currentSec = currentTime * 0.001;  // Convert current time to seconds
    //     float mult = 2 * PI * currentSec;

    //     // Calculate amplitude based on sum of sines
    //     amplitude = weight0 * sin(mult * freq0) + weight1 * sin(mult * freq1);

    //     // Ensure amplitude is within DAC limits
    //     float dacValue = min(DAC_MAX, max(DAC_MIN, amplitude));

    //     // Set DAC output
    //     dac.setVoltage(dacValue);

    //     // Set timing for the next step
    //     nextStepTime = currentTime + stepSize;
        
    //     // Delay until the next step time
    //     while (millis() < nextStepTime) {
    //         printCurrent();  // Monitor or log current
    //     }
    // }
}

void ArchStimV3::rampedSine(float rampFreq, float duration, float weight0, float freq0, int stepSize) {
    // unsigned long startTime = millis();
    // unsigned long currentTime;
    // float currentSec;
    // float startSec = startTime * 0.001;
    // float amplitude;
    // float zSetCurr = 0;
    // float zSetR = 0;
    // bool newZ = false;

    // Z = zCheck();  // Get impedance (if needed)

    // // Loop for the specified duration
    // while (millis() - startTime < duration) {
    //     currentTime = millis();
    //     currentSec = currentTime * 0.001;  // Convert to seconds for frequency calculations
    //     float mult = 2 * PI * (currentSec - startSec);
        
    //     // Calculate amplitude using ramped sine and secondary sine with given frequencies
    //     amplitude = weight0 * sin(mult * rampFreq) + 0.5 * sin(mult * freq0);

    //     // Clamp amplitude within DAC range
    //     float dacValue = min(DAC_MAX, max(DAC_MIN, amplitude));

    //     // Set DAC output
    //     dac.setVoltage(dacValue);

    //     // Set timing for the next step
    //     unsigned long nextStepTime = currentTime + stepSize;
        
    //     // Delay until the next step time while monitoring current
    //     while (millis() < nextStepTime) {
    //         printCurrent();  // Log or monitor current

    //         // Example of tracking current changes if you need it for impedance updates
    //         if (abs(current) > 0.5) {  // Replace 'current' with actual current-reading logic
    //             zSetCurr = current;
    //             zSetR = dacValue;
    //             newZ = true;
    //         }
    //     }
    // }

    // // Experimental impedance calculation (modify as needed)
    // if (newZ) {
    //     Z = (27.0 / (zSetCurr * 0.001)) - 1000.0 - (abs(zSetR * (200000.0 / DAC_MAX)));
    //     newZ = false;
    // }
}

float ArchStimV3::zCheck() {
    return 0.0;
}

void ArchStimV3::printCurrent() {
    Serial.println("Current = ...");
}