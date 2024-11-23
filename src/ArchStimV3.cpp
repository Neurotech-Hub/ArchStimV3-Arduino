#include "ArchStimV3.h"

ArchStimV3::ArchStimV3() : adc(ADC_CS), dac(DAC_CS, VREF), activeWaveform(nullptr) {}

void ArchStimV3::begin()
{
    initPins();
    initSPI();
    initI2C();
}

void ArchStimV3::initPins()
{
    pinMode(USB_SENSE, INPUT);
    pinMode(USER_IN, INPUT_PULLUP);

    pinMode(LED_R, OUTPUT);
    digitalWrite(LED_R, LOW);

    pinMode(EXT_OUTPUT, OUTPUT);
    digitalWrite(EXT_OUTPUT, LOW);

    pinMode(BUZZ, OUTPUT);

    pinMode(EXT_INPUT, INPUT);

    pinMode(ADC_CS, OUTPUT);
    digitalWrite(ADC_CS, HIGH); // Disable ADC by default

    pinMode(DAC_CS, OUTPUT);
    digitalWrite(DAC_CS, HIGH); // Disable DAC by default

    pinMode(LED_STIM, OUTPUT);
    digitalWrite(LED_STIM, LOW);

    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH); // Disable SD by default

    pinMode(FUEL_ALERT, INPUT);

    pinMode(DISABLE, OUTPUT);
    digitalWrite(DISABLE, LOW);

    pinMode(LED_B, OUTPUT);

    pinMode(DRIVE_EN, OUTPUT);

    activateIsolated(); // Ensure isolated components are activated (esp. for DAC/ADC)
}

void ArchStimV3::initSPI()
{
    SPI.begin(SCK, MISO, MOSI, SD_CS);
}

void ArchStimV3::initI2C()
{
    Wire.begin(SDA_PIN, SCL_PIN);
}

void ArchStimV3::initADC()
{
    adc.begin();
    adc.setSingleShotMode();
    adc.setSamplingRate(ADS1118::RATE_128SPS);
    adc.setInputSelected(ADS1118::AIN_0);
    adc.setFullScaleRange(ADS1118::FSR_4096);
}

void ArchStimV3::initDAC()
{
    dac.setup(AD57X4R::AD5754R);
    dac.setAllOutputRanges(AD57X4R::BIPOLAR_5V);
    dac.setAllVoltages(0);
}

void ArchStimV3::activateIsolated()
{
    digitalWrite(DRIVE_EN, HIGH);
    delay(500);
    initADC();
    initDAC();
}

void ArchStimV3::deactivateIsolated()
{
    digitalWrite(DRIVE_EN, LOW);
}

void ArchStimV3::enableStim()
{
    dac.setAllVoltages(0);
    digitalWrite(DISABLE, LOW);
    digitalWrite(LED_STIM, HIGH);
}

void ArchStimV3::disableStim()
{
    dac.setAllVoltages(0);
    digitalWrite(DISABLE, HIGH);
    digitalWrite(LED_STIM, LOW);
}

void ArchStimV3::beep(int frequency, int duration)
{
    tone(BUZZ, frequency, duration);
}

// Sets a new active waveform, deleting any previous one
void ArchStimV3::setActiveWaveform(Waveform *waveform)
{
    if (activeWaveform)
    {
        delete activeWaveform; // Clean up the existing waveform
    }
    activeWaveform = waveform; // Assign the new waveform
}

// Runs the active waveform if one is set
void ArchStimV3::runWaveform()
{
    if (activeWaveform)
    {
        activeWaveform->execute(); // Call the execute function of the active waveform
    }
}

// Generates a square wave with specified negative and positive voltages at given frequency
// @param negVal: negative voltage value (V)
// @param posVal: positive voltage value (V)
// @param frequency: wave frequency (Hz)
// Example: square(-2.0, 2.0, 10.0) // ±2V square wave at 10Hz
//
// Square Wave Pattern:
//
// Time:      0ms    50ms   100ms  150ms  200ms
//           |      |      |      |      |
// Voltage:   2V     -2V    2V     -2V    2V
//           ┌──────┐      ┌──────┐      ┌────
//           │      │      │      │      │
//           │      │      │      │      │
//           │      └──────┘      └──────┘
//          -2V
//
// Details:
// Period:    100ms (10Hz)
// Duty:      50%
// States:    Alternates between posVal and negVal
//
void ArchStimV3::square(float negVal, float posVal, float frequency)
{
    static bool highState = false;
    static unsigned long lastToggleTime = 0;
    unsigned long interval = 1000000 / (2 * frequency); // Half-period in microseconds

    unsigned long currentTime = micros();
    if (currentTime - lastToggleTime >= interval)
    {
        lastToggleTime = currentTime;
        highState = !highState;

        // Set DAC output based on state
        dac.setAllVoltages(highState ? posVal : negVal);
    }
}

// Generates a pulse train using arrays of amplitudes and durations
// @param ampArray: array of voltage values (V)
// @param timeArray: array of durations (ms). If timeArray[1]=0, timeArray[0] is used for all amplitudes
// @param arrSize: size of ampArray (timeArray must be either size 1 or arrSize)
// Example 1: float amp[]={0,2,-2}; int time[]={25,50,200}; pulse(amp, time, 3) // Different durations
// Example 2: float amp[]={0,2,-2}; int time[]={100,0}; pulse(amp, time, 3) // 100ms each

// Multiple Duration Mode:
// Time:      0ms     25ms    75ms    275ms   300ms
//            |       |       |       |       |
// Voltage:   0V      2V      -2V     0V      2V
//            ├───────┼───────┼───────┼───────┤
// Duration:  |--25ms-|--50ms-|-200ms-|--25ms-|...

// Array View:
// ampArray:  [0V]---->[2V]---->[-2V]--->[0V]---(repeat)
// timeArray: [25ms]-->[50ms]-->[200ms]->[25ms]-(repeat)

// Single Duration Mode (timeArray[1]=0):
// Time:      0ms     100ms   200ms   300ms   400ms
//            |       |       |       |       |
// Voltage:   0V      2V      -2V     0V      2V
//            ├───────┼───────┼───────┼───────┤
// Duration:  |-100ms-|-100ms-|-100ms-|-100ms-|...

// Array View:
// ampArray:  [0V]---->[2V]---->[-2V]--->[0V]---(repeat)
// timeArray: [100ms]--┴--------┴--------┴-------(shared)
void ArchStimV3::pulse(float ampArray[], int timeArray[], int arrSize)
{
    static int currentIndex = 0;
    static unsigned long lastTransitionTime = 0;

    unsigned long currentTime = millis();

    // Check if it's time to transition to the next value
    if (currentTime - lastTransitionTime >= timeArray[currentIndex])
    {
        // Move to next index, wrapping around to 0 if we reach the end
        currentIndex = (currentIndex + 1) % arrSize;
        lastTransitionTime = currentTime;

        // Set the new voltage
        dac.setAllVoltages(ampArray[currentIndex]);
    }
}

// Generates random pulses alternating between 0V and random values from ampArray
// Zero state lasts 1000-1500ms, active state lasts either 25ms or 100ms
// @param ampArray: array of possible voltage values (V)
// @param arrSize: size of ampArray
// Example: float amp[]={-2,2,1.5,-1.5}; randPulse(amp, 4) // Random ±1.5V or ±2V pulses
//
// Random Pulse Wave Pattern:
//
// Time:      0ms     1200ms  1225ms  2725ms  2825ms   4325ms
//           |       |       |       |       |       |
// Voltage:   0V      2V      0V      -1.5V   0V      1.5V
//           ├───────┼───────┼───────┼───────┼───────┤
// Duration:  |--1200ms--|-25ms-|-1500ms-|-100ms|-1500ms-|...
// State:     |---ZERO---|-ACT-|--ZERO--|--ACT--|--ZERO--|...
//
// Details:
// ZERO state:
// - Always 0V
// - Duration: 1000-1500ms (random)
//
// ACTIVE state:
// - Random voltage from ampArray
// - Duration: either 25ms or 100ms (random)
//
// Array View:
// ampArray: [2V, -2V, 1.5V, -1.5V] (random selection each active state)
void ArchStimV3::randPulse(float ampArray[], int arrSize)
{
    static bool inZeroState = true;
    static unsigned long lastTransitionTime = 0;
    static unsigned long currentDuration = 1000;
    static float currentAmplitude = 0;

    unsigned long currentTime = millis();

    if (currentTime - lastTransitionTime >= currentDuration)
    {
        lastTransitionTime = currentTime;

        if (inZeroState)
        {
            // Transition to active state
            inZeroState = false;
            currentAmplitude = ampArray[random(arrSize)];
            currentDuration = (random(2) == 0) ? 25 : 100;
        }
        else
        {
            // Transition to zero state
            inZeroState = true;
            currentAmplitude = 0;
            currentDuration = 1000 + random(501);
        }

        dac.setAllVoltages(currentAmplitude);
    }
}

// !! DEPRECATED: simply pass single timeArr element to pulse()
// void ArchStimV3::readTimeSeries(float ampArray[], int arrSize, int stepSize)
// {
//     // Implement time series reading logic
// }

// Generates a sum of two sine waves with specified weights, frequencies and duration
// @param stepSize: update interval (ms)
// @param weight0: amplitude of first sine wave (V)
// @param freq0: frequency of first sine wave (Hz)
// @param weight1: amplitude of second sine wave (V)
// @param freq1: frequency of second sine wave (Hz)
// @param duration: total duration (ms), 0 for infinite
// Example: sumOfSines(1, 2.0, 10.0, 1.0, 20.0, 1000) // 2V@10Hz + 1V@20Hz for 1s
//
// Sum of Sines Wave Pattern:
//
// Time:      0ms    25ms   50ms   75ms   100ms
//           |      |      |      |      |
// Voltage:   3V                           3V
//           ┌                             ┐
//           │    Combined Waveform        │
//    2V ────┤      = sin(2π×10t)         ├──── 2V
//           │      + 0.5×sin(2π×20t)     │
//    1V ────┤                            ├──── 1V
//           │                            │
//    0V ────┼────────────────────────────┼──── 0V
//           │                            │
//   -1V ────┤                            ├────-1V
//           │                            │
//   -2V ────┤                            ├────-2V
//           │                            │
//   -3V     └                            ┘    -3V
//
// Details:
// - Combines two sine waves with different frequencies
// - Total voltage is sum of both waves
// - Updates every stepSize milliseconds
// - Runs for specified duration or indefinitely if duration=0
//
// Parameters View:
// weight0=2V, freq0=10Hz  -> Primary sine wave
// weight1=1V, freq1=20Hz  -> Secondary sine wave
// Combined peak voltage = |weight0| + |weight1|
void ArchStimV3::sumOfSines(int stepSize, float weight0, float freq0, float weight1, float freq1, int duration)
{
    static unsigned long startTime = millis();
    static unsigned long lastStepTime = 0;
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - startTime;

    // Check if the waveform duration has elapsed
    if (duration > 0 && elapsedTime >= (unsigned long)duration)
    {
        dac.setAllVoltages(0); // Reset to 0V
        return;
    }

    // Only update at specified step intervals
    if (currentTime - lastStepTime >= stepSize)
    {
        // Calculate time in seconds for sine functions
        float t = elapsedTime / 1000.0f;

        // Calculate the sum of sines
        float value = weight0 * sin(2 * PI * freq0 * t) +
                      weight1 * sin(2 * PI * freq1 * t);

        // Update the output
        dac.setAllVoltages(value);
        lastStepTime = currentTime;
    }
}

// Generates a sine wave with amplitude that ramps up and down
// @param rampFreq: frequency of amplitude ramping (Hz)
// @param duration: total duration of waveform (ms)
// @param weight0: maximum amplitude of sine wave (V)
// @param freq0: frequency of sine wave (Hz)
// @param stepSize: update interval (ms)
// Example: rampedSine(0.5, 2000, 2.0, 10.0, 1) // 2V max, 10Hz sine, 0.5Hz ramp, 2s
//
// Ramped Sine Wave Pattern:
//
// Time:      0ms    500ms  1000ms 1500ms 2000ms
//           |      |      |      |      |
// Voltage:   2V     Envelope of amplitude     2V
//           ┌─┐                             ┌─┐
//           │ │    Ramped Sine Wave        │ │
//    1V ────┤ └──┐                     ┌───┘ ├──── 1V
//           │    │                     │     │
//    0V ────┼────┼─────────────────────┼─────┼──── 0V
//           │    │                     │     │
//   -1V ────┤ ┌──┘                     └───┐ ├────-1V
//           │ │                             │ │
//   -2V     └─┘                             └─┘    -2V
//
// Details:
// - Base sine wave at freq0
// - Amplitude modulated by slower ramp at rampFreq
// - Updates every stepSize milliseconds
// - Runs for specified duration
//
// Parameters View:
// rampFreq=0.5Hz -> Complete ramp cycle every 2 seconds
// weight0=2V     -> Maximum amplitude of ±2V
// freq0=10Hz     -> Base sine wave frequency
void ArchStimV3::rampedSine(float rampFreq, float duration, float weight0, float freq0, int stepSize)
{
    static unsigned long startTime = millis();
    static unsigned long lastStepTime = 0;
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - startTime;

    // Check if the waveform duration has elapsed
    if (duration > 0 && elapsedTime >= (unsigned long)duration)
    {
        dac.setAllVoltages(0); // Reset to 0V
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
        dac.setAllVoltages(value);
        lastStepTime = currentTime;
    }
}

// Utility functions for impedance and current measurement
float ArchStimV3::zCheck()
{
    // Implement impedance check logic
    return Z;
}

void ArchStimV3::printCurrent()
{
    // Implement current print logic
}
