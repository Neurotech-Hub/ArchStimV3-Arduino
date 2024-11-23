# ArchStimV3 Library

`ArchStimV3` is an Arduino library designed to control and generate complex waveforms with an ADC (ADS1118) and a DAC (AD57X4R). This library allows for the generation of various waveforms and provides utility functions for managing and controlling isolated components.

## Features

- **Waveform Generation**: Square waves, pulse sequences, and random pulses
- **Object-Oriented Design**: Extensible waveform class hierarchy
- **Active Waveform Management**: Dynamic waveform switching with proper cleanup
- **Isolated Component Control**: Functions for managing isolated hardware
- **Hardware Integration**: ADC and DAC control with safety features

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

### Input Pins
| Pin Name    | Pin Number | Description             | Default State |
|-------------|------------|-------------------------|---------------|
| USB_SENSE   | 1         | USB power sense         | INPUT        |
| USER_IN     | 2         | User input              | INPUT_PULLUP |
| EXT_INPUT   | 11        | External input          | INPUT        |
| FUEL_ALERT  | 42        | Fuel gauge alert        | INPUT        |

### Output Pins
| Pin Name    | Pin Number | Description             | Default State |
|-------------|------------|-------------------------|---------------|
| LED_R       | 45        | Red status LED          | LOW          |
| LED_B       | 5         | Blue status LED         | -            |
| LED_STIM    | 46        | Stimulation indicator   | LOW          |
| EXT_OUTPUT  | 9         | External output control | LOW          |
| BUZZ        | 10        | Buzzer output           | -            |
| DISABLE     | 41        | Output disable control  | LOW          |
| DRIVE_EN    | 40        | Isolation control       | HIGH*        |

### Communication Pins
| Pin Name    | Pin Number | Description             | Default State |
|-------------|------------|-------------------------|---------------|
| ADC_CS      | 12        | ADC chip select         | HIGH         |
| DAC_CS      | 13        | DAC chip select         | HIGH         |
| SD_CS       | 34        | SD card chip select     | HIGH         |
| MISO        | 37        | SPI MISO                | -            |
| MOSI        | 35        | SPI MOSI                | -            |
| SCK         | 36        | SPI SCK                 | -            |
| SDA_PIN     | 3         | I2C data                | -            |
| SCL_PIN     | 4         | I2C clock               | -            |

\* Set HIGH during initialization via `activateIsolated()`

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

## Available Waveforms

### Square Wave
- Parameters: negative voltage, positive voltage, frequency
- Generates alternating voltage between negVal and posVal at specified frequency
- Example: `±2V at 10Hz`

### Pulse Wave
- Parameters: amplitude array, time array, array size
- Supports two modes:
  - Multiple durations: Each amplitude has its own duration
  - Single duration: One duration for all amplitudes (set timeArray[1]=0)
- Example: `[0V, 2V, -2V] for [25ms, 50ms, 200ms]`

### Random Pulse Wave
- Parameters: amplitude array, array size
- Zero state: 1000-1500ms at 0V
- Active state: Randomly selects amplitude and duration (25ms or 100ms)
- Example: `Random selection from [-2V, 2V, 1.5V, -1.5V]`

## License

This library is provided under the MIT License. See `LICENSE` for details.