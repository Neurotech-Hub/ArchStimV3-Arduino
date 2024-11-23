#include "ArchStimV3.h"

ArchStimV3::ArchStimV3() : adc(ADC_CS), dac(DAC_CS, VREF), activeWaveform(nullptr) {}

void ArchStimV3::begin()
{
    initPins();
    initSPI();
    initI2C();
    initADC();
    initDAC();
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
}

void ArchStimV3::deactivateIsolated()
{
    digitalWrite(DRIVE_EN, LOW);
}

void ArchStimV3::enableStim()
{
    digitalWrite(DISABLE, LOW);
    digitalWrite(LED_STIM, HIGH);
}

void ArchStimV3::disableStim()
{
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

// !! these settings appear to be hard coded
void ArchStimV3::sumOfSines(int stepSize, float weight0, float freq0, float weight1, float freq1, int duration)
{
    // Implement sum of sines logic
}

void ArchStimV3::rampedSine(float rampFreq, float duration, float weight0, float freq0, int stepSize)
{
    // Implement ramped sine wave logic
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
