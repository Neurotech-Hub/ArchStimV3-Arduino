#include "ArchStimV3.h"

ArchStimV3::ArchStimV3() : adc(ADC_CS), dac(DAC_CS, VREF), activeWaveform(nullptr) {}

void ArchStimV3::begin() {
    initPins();
    initSPI();
    initI2C();
    initADC();
    initDAC();
}

void ArchStimV3::initPins() {
    pinMode(USB_SENSE, INPUT);
    pinMode(USER_IN, INPUT_PULLUP);

    pinMode(LED_R, OUTPUT);
    digitalWrite(LED_R, LOW);

    pinMode(EXT_OUTPUT, OUTPUT);
    digitalWrite(EXT_OUTPUT, LOW);

    pinMode(BUZZ, OUTPUT);

    pinMode(EXT_INPUT, INPUT);

    pinMode(ADC_CS, OUTPUT);
    digitalWrite(ADC_CS, HIGH);  // Disable ADC by default

    pinMode(DAC_CS, OUTPUT);
    digitalWrite(DAC_CS, HIGH);  // Disable DAC by default

    pinMode(LED_STIM, OUTPUT);
    digitalWrite(LED_STIM, LOW);

    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);  // Disable SD by default

    pinMode(FUEL_ALERT, INPUT);

    pinMode(DISABLE, OUTPUT);
    digitalWrite(DISABLE, LOW);

    pinMode(LED_B, OUTPUT);

    pinMode(DRIVE_EN, OUTPUT);

    activateIsolated();  // Ensure isolated components are activated (esp. for DAC/ADC)
}

void ArchStimV3::initSPI() {
    SPI.begin(SCK, MISO, MOSI, SD_CS);
}

void ArchStimV3::initI2C() {
    Wire.begin(SDA_PIN, SCL_PIN);
}

void ArchStimV3::initADC() {
    adc.begin();
    adc.setSingleShotMode();
    adc.setSamplingRate(ADS1118::RATE_128SPS);
    adc.setInputSelected(ADS1118::AIN_0);
    adc.setFullScaleRange(ADS1118::FSR_4096);
}

void ArchStimV3::initDAC() {
    dac.setup(AD57X4R::AD5754R);
    dac.setAllOutputRanges(AD57X4R::BIPOLAR_5V);
    dac.setAllVoltages(0);
}

void ArchStimV3::activateIsolated() {
    digitalWrite(DRIVE_EN, HIGH);
    delay(500);
}

void ArchStimV3::deactivateIsolated() {
    digitalWrite(DRIVE_EN, LOW);
}

void ArchStimV3::enableStim() {
    digitalWrite(DISABLE, LOW);
    digitalWrite(LED_STIM, HIGH);
}

void ArchStimV3::disableStim() {
    digitalWrite(DISABLE, HIGH);
    digitalWrite(LED_STIM, LOW);
}

void ArchStimV3::beep(int frequency, int duration) {
    tone(BUZZ, frequency, duration);
}

// Sets a new active waveform, deleting any previous one
void ArchStimV3::setActiveWaveform(Waveform* waveform) {
    if (activeWaveform) {
        delete activeWaveform;  // Clean up the existing waveform
    }
    activeWaveform = waveform;  // Assign the new waveform
}

// Runs the active waveform if one is set
void ArchStimV3::runWaveform() {
    if (activeWaveform) {
        activeWaveform->execute();  // Call the execute function of the active waveform
    }
}

// Waveform generation functions for specific waveforms
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
    // Implement pulse generation logic using ampArray and timeArray
}

void ArchStimV3::randPulse(float ampArray[], int arrSize) {
    // Implement random pulse logic using ampArray
}

void ArchStimV3::readTimeSeries(float ampArray[], int arrSize, int stepSize) {
    // Implement time series reading logic
}

void ArchStimV3::sumOfSines(int stepSize, float weight0, float freq0, float weight1, float freq1, int duration) {
    // Implement sum of sines logic
}

void ArchStimV3::rampedSine(float rampFreq, float duration, float weight0, float freq0, int stepSize) {
    // Implement ramped sine wave logic
}

// Utility functions for impedance and current measurement
float ArchStimV3::zCheck() {
    // Implement impedance check logic
    return Z;
}

void ArchStimV3::printCurrent() {
    // Implement current print logic
}
