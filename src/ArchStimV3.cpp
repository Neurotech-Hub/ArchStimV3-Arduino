#include "ArchStimV3.h"
#include "CommandInterpreter.h"
#include "esp_system.h"
#include "esp_bt.h"
#include "esp_mac.h"

// Initialize static members
volatile unsigned long ArchStimV3::lastDebounceTime = 0;
ArchStimV3 *ArchStimV3::instance = nullptr;

ArchStimV3::ArchStimV3() : adc(ADC_CS), dac(DAC_CS, VREF), activeWaveform(nullptr)
{
    instance = this; // Store instance for ISR
}

void IRAM_ATTR smartIntISR()
{
    digitalWrite(PS_HOLD, LOW);
}

void IRAM_ATTR ArchStimV3::userButtonISR()
{
    unsigned long currentTime = millis();
    if (currentTime - lastDebounceTime > debounceDelay)
    {
        lastDebounceTime = currentTime;
        if (instance)
        {
            instance->handleUserButton();
        }
    }
}

void ArchStimV3::handleUserButton()
{
    if (digitalRead(DRIVE_EN) == HIGH)
    {
        deactivateIsolated();
    }
    else
    {
        activateIsolated();
    }
}

void ArchStimV3::begin()
{
    initPins();
    initSPI();
    initI2C();
    initSD();

    // Fun startup melody
    beep(1047, 100); // C6
    delay(50);
    beep(1319, 100); // E6
    delay(50);
    beep(1568, 150); // G6
    delay(100);
    beep(2093, 200); // C7

    disableStim();
    setRedLED();
}

void ArchStimV3::initPins()
{
    pinMode(DRIVE_EN, OUTPUT);
    digitalWrite(DRIVE_EN, LOW); // deactivate drive

    pinMode(DISABLE, OUTPUT);
    digitalWrite(DISABLE, LOW); // disable stim

    pinMode(USB_SENSE, INPUT);
    pinMode(USER_IN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(USER_IN), userButtonISR, FALLING);

    pinMode(LED_R, OUTPUT);
    digitalWrite(LED_R, LOW);

    pinMode(EXT_OUTPUT, OUTPUT);
    digitalWrite(EXT_OUTPUT, LOW);

    pinMode(PS_HOLD, OUTPUT);
    digitalWrite(PS_HOLD, HIGH); // allows VIN EN

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

    pinMode(LED_B, OUTPUT);
    digitalWrite(LED_B, LOW);

    pinMode(RTC_INT, INPUT_PULLUP);
    pinMode(FUEL_ALERT, INPUT_PULLUP);

    pinMode(SMART_INT, INPUT);
    attachInterrupt(SMART_INT, smartIntISR, FALLING); // Trigger on FALLING edge (HIGH to LOW)
}

void ArchStimV3::initSPI()
{
    SPI.begin(SCK, MISO, MOSI, SD_CS);
}

void ArchStimV3::initI2C()
{
    Wire.begin(SDA_PIN, SCL_PIN);
}

void ArchStimV3::initSD()
{
    if (!SD.begin(SD_CS))
    {
        Serial.println("SD card initialization failed!");
    }
}

void ArchStimV3::initADC()
{
    adc.begin();
    adc.setSamplingRate(ADS1118::RATE_128SPS);
    adc.setInputSelected(ADS1118::AIN_0);
    adc.setFullScaleRange(ADS1118::FSR_4096);
    adc.setContinuousMode();
    adc.disablePullup();
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
    setRedLED();
}

void ArchStimV3::deactivateIsolated()
{
    digitalWrite(DRIVE_EN, LOW);
    setRedLED();
}

void ArchStimV3::enableStim()
{
    setAllCurrents(0);
    digitalWrite(DISABLE, LOW);
    digitalWrite(LED_STIM, HIGH);
    setRedLED();
}

void ArchStimV3::disableStim()
{
    setAllCurrents(0);
    digitalWrite(DISABLE, HIGH);
    digitalWrite(LED_STIM, LOW);
    setRedLED();
}

void ArchStimV3::setRedLED()
{
    if (digitalRead(DRIVE_EN) == HIGH && digitalRead(DISABLE) == HIGH)
    {
        digitalWrite(LED_R, HIGH);
    }
    else
    {
        digitalWrite(LED_R, LOW);
    }
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

void ArchStimV3::setCSDelay(uint16_t delay)
{
    adc.csDelay = delay;
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
void ArchStimV3::square(int negVal, int posVal, float frequency)
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
        setAllCurrents(highState ? posVal : negVal);
    }
}

// Generates a pulse train using arrays of amplitudes and durations
// @param ampArray: array of current values (µA)
// @param timeArray: array of durations (ms). Can be either:
//                  - Single value: same duration used for all amplitudes
//                  - Same size as ampArray: each duration corresponds to its amplitude
// @param arrSize: size of ampArray
// Example 1: int amp[]={0,2000,-2000}; int time[]={25,50,200}; pulse(amp, time, 3) // Different durations
// Example 2: int amp[]={0,2000,-2000}; int time[]={100}; pulse(amp, time, 3) // 100ms each

// Multiple Duration Mode:
// Time:      0ms     25ms    75ms    275ms   300ms
//            |       |       |       |       |
// Current:   0µA     2000µA  -2000µA 0µA     2000µA
//            ├───────┼───────┼───────┼───────┤
// Duration:  |--25ms-|--50ms-|-200ms-|--25ms-|...

// Array View:
// ampArray:  [0µA]────>[2000µA]────>[-2000µA]────>[0µA]──(repeat)
// timeArray: [25ms]───>[50ms]────>[200ms]────>[25ms]─(repeat)

// Single Duration Mode:
// Time:      0ms     100ms   200ms   300ms   400ms
//            |       |       |       |       |
// Current:   0µA     2000µA  -2000µA 0µA     2000µA
//            ├───────┼───────┼───────┼───────┤
// Duration:  |-100ms-|-100ms-|-100ms-|-100ms-|...

// Array View:
// ampArray:  [0µA]────>[2000µA]────>[-2000µA]────>[0µA]──(repeat)
// timeArray: [100ms]  (same duration for all values)

void ArchStimV3::pulse(int ampArray[], int timeArray[], int arrSize)
{
    static int currentIndex = 0;
    static unsigned long lastTransitionTime = 0;

    unsigned long currentTime = millis();

    // Get the duration for the current index
    // If timeArray has only one value, use that for all indices
    int duration = (timeArray[1] == 0) ? timeArray[0] : timeArray[currentIndex];

    // Check if it's time to transition to the next value
    if (currentTime - lastTransitionTime >= duration)
    {
        // Move to next index, wrapping around to 0 if we reach the end
        currentIndex = (currentIndex + 1) % arrSize;
        lastTransitionTime = currentTime;

        // Set the new current
        setAllCurrents(ampArray[currentIndex]);
    }
}

// Generates random pulses alternating between 0µA and random values from ampArray
// Zero state lasts 1000-1500ms, active state lasts either 25ms or 100ms
// @param ampArray: array of possible current values (µA)
// @param arrSize: size of ampArray
// Example: int amp[]={-2000,2000,1500,-1500}; randPulse(amp, 4) // Random ±1500µA or ±2000µA pulses
//
// Random Pulse Wave Pattern:
//
// Time:      0ms     1200ms  1225ms  2725ms  2825ms   4325ms
//            |       |       |       |       |       |
// Current:   0µA     2000µA  0µA     -1500µA 0µA     1500µA
//            ├───────┼───────┼───────┼───────┼───────┤
// Duration:  |--1200ms--|-25ms-|-1500ms-|-100ms|-1500ms-|...
// State:     |---ZERO---|-ACT-|--ZERO--|--ACT--|--ZERO--|...
//
// Details:
// ZERO state:
// - Always 0µA
// - Duration: 1000-1500ms (random)
//
// ACTIVE state:
// - Random current from ampArray
// - Duration: either 25ms or 100ms (random)
//
// Array View:
// ampArray: [2000µA]──[−2000µA]──[1500µA]──[−1500µA] (random selection each active state)
void ArchStimV3::randPulse(int ampArray[], int arrSize)
{
    static bool inZeroState = true;
    static unsigned long lastTransitionTime = 0;
    static unsigned long currentDuration = 1000;
    static int currentAmplitude = 0;

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

        setAllCurrents(currentAmplitude);
    }
}

// !! DEPRECATED: simply pass single timeArr element to pulse()
// void ArchStimV3::readTimeSeries(float ampArray[], int arrSize, int stepSize)
// {
//     // Implement time series reading logic
// }

// Generates a sum of two sine waves with specified weights, frequencies and duration
// @param stepSize: update interval (ms)
// @param weight0: amplitude of first sine wave (µA)
// @param freq0: frequency of first sine wave (Hz)
// @param weight1: amplitude of second sine wave (µA)
// @param freq1: frequency of second sine wave (Hz)
// @param duration: total duration (ms), 0 for infinite
// Example: sumOfSines(1, 2000, 10.0, 1000, 20.0, 1000) // 2000µA@10Hz + 1000µA@20Hz for 1s
//
// Sum of Sines Wave Pattern:
//
// Time:      0ms    25ms   50ms   75ms   100ms
//            |      |      |      |      |
// Current:   3000µA                            3000µA
//            ┌──────────────────────────────┐
//            │    Combined Waveform         │
//    2000µA ─┤    = sin(2π×10t)×2000µA      ├─── 2000µA
//            │    + sin(2π×20t)×1000µA      │
//    1000µA ─┤                              ├─── 1000µA
//            │                              │
//       0µA ─┼──────────────────────────────┼─── 0µA
//            │                              │
//   -1000µA ─┤                              ├─── -1000µA
//            │                              │
//   -2000µA ─┤                              ├─── -2000µA
//            │                              │
//   -3000µA  └──────────────────────────────┘    -3000µA
//
// Details:
// - Combines two sine waves with different frequencies
// - Total current is sum of both waves
// - Updates every stepSize milliseconds
// - Runs for specified duration or indefinitely if duration=0
//
// Parameters View:
// weight0=2000µA, freq0=10Hz  -> Primary sine wave
// weight1=1000µA, freq1=20Hz  -> Secondary sine wave
// Combined peak current = |weight0| + |weight1|
void ArchStimV3::sumOfSines(int stepSize, float weight0, float freq0, float weight1, float freq1, int duration)
{
    static unsigned long startTime = millis();
    static unsigned long lastStepTime = 0;
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - startTime;

    // Check if the waveform duration has elapsed
    if (duration > 0 && elapsedTime >= (unsigned long)duration)
    {
        setAllCurrents(0);
        return;
    }

    // Only update at specified step intervals
    if (currentTime - lastStepTime >= stepSize)
    {
        // Calculate time in seconds for sine functions
        float t = elapsedTime / 1000.0f;

        // Calculate the sum of sines (values are in microamps)
        float value = weight0 * sin(2 * PI * freq0 * t) +
                      weight1 * sin(2 * PI * freq1 * t);

        // Update the output
        setAllCurrents(static_cast<int>(value));
        lastStepTime = currentTime;
    }
}

// Generates a sine wave with amplitude that ramps up and down
// @param rampFreq: frequency of amplitude ramping (Hz)
// @param duration: total duration of waveform (ms)
// @param weight0: maximum amplitude of sine wave (µA)
// @param freq0: frequency of sine wave (Hz)
// @param stepSize: update interval (ms)
// Example: rampedSine(0.5, 2000, 2000, 10.0, 1) // 2000µA max, 10Hz sine, 0.5Hz ramp, 2s
//
// Ramped Sine Wave Pattern:
//
// Time:      0ms    500ms  1000ms 1500ms 2000ms
//            |      |      |      |      |
// Current:   2000µA Envelope of amplitude     2000µA
//            ┌─┐                             ┌─┐
//            │ │    Ramped Sine Wave         │ │
//    1000µA ─┤ └──┐                       ┌──┘ ├─── 1000µA
//            │    │                       │    │
//       0µA ─┼────┼───────────────────────┼────┼─── 0µA
//            │    │                       │    │
//   -1000µA ─┤ ┌──┘                       └──┐ ├─── -1000µA
//            │ │                             │ │
//   -2000µA  └─┘                             └─┘    -2000µA
//
// Details:
// - Base sine wave at freq0
// - Amplitude modulated by slower ramp at rampFreq
// - Updates output current every stepSize milliseconds
// - Runs for specified duration
//
// Parameters View:
// rampFreq=0.5Hz  -> Complete ramp cycle every 2 seconds
// weight0=2000µA  -> Maximum amplitude of ±2000µA
// freq0=10Hz      -> Base sine wave frequency
void ArchStimV3::rampedSine(float rampFreq, float duration, float weight0, float freq0, int stepSize)
{
    static unsigned long startTime = millis();
    static unsigned long lastStepTime = 0;
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - startTime;

    // Check if the waveform duration has elapsed
    if (duration > 0 && elapsedTime >= (unsigned long)duration)
    {
        setAllCurrents(0);
        return;
    }

    // Only update at specified step intervals
    if (currentTime - lastStepTime >= stepSize)
    {
        // Calculate time in seconds for sine functions
        float t = elapsedTime / 1000.0f;

        // Calculate the ramped sine wave (values are in microamps)
        float envelope = abs(sin(PI * rampFreq * t));
        float value = weight0 * envelope * sin(2 * PI * freq0 * t);

        // Update the output
        setAllCurrents(static_cast<int>(value));
        lastStepTime = currentTime;
    }
}

// Utility functions for impedance and current measurement
void ArchStimV3::zCheck()
{
    double mv = getMilliVolts(0);
    Serial.print("mV: ");
    Serial.println(mv);
    Z = mv; // Store in member variable instead of returning
}

double ArchStimV3::getMilliVolts(uint8_t channel)
{
    // Map channel 0-3 to ADS1118 single-ended inputs
    uint8_t adsChannel;
    switch (channel)
    {
    case 0:
        adsChannel = ADS1118::AIN_0;
        break;
    case 1:
        adsChannel = ADS1118::AIN_1;
        break;
    case 2:
        adsChannel = ADS1118::AIN_2;
        break;
    case 3:
        adsChannel = ADS1118::AIN_3;
        break;
    default:
        adsChannel = ADS1118::AIN_0; // Default to channel 0 if invalid input
    }
    return adc.getMilliVolts(adsChannel);
}

void ArchStimV3::setVoltage(float voltage)
{
    dac.setAllVoltages(voltage);
}

uint16_t ArchStimV3::getRawADC(uint8_t channel)
{
    // Map channel 0-3 to ADS1118 single-ended inputs
    uint8_t adsChannel;
    switch (channel)
    {
    case 0:
        adsChannel = ADS1118::AIN_0;
        break;
    case 1:
        adsChannel = ADS1118::AIN_1;
        break;
    case 2:
        adsChannel = ADS1118::AIN_2;
        break;
    case 3:
        adsChannel = ADS1118::AIN_3;
        break;
    default:
        adsChannel = ADS1118::AIN_0; // Default to channel 0 if invalid input
    }
    return adc.getADCValue(adsChannel);
}

// Transfer Function (V as a function of uA): -1.115e-03*uA + -2.189e-05
// see: /Users/gaidica/Documents/MATLAB/Ching Lab/ARCHv3_IV.m
void ArchStimV3::setAllCurrents(int microAmps)
{
    // Clamp the input between -2000 and 2000 µA
    microAmps = constrain(microAmps, -MAX_CURRENT, MAX_CURRENT);

    // Convert microamps to voltage using transfer function
    float voltage = -1.115e-03f * microAmps + -2.189e-05f;

    // Set DAC output
    dac.setAllVoltages(voltage);
}

// BLE Server Callbacks
class MyServerCallbacks : public BLEServerCallbacks
{
private:
    ArchStimV3 &device;

public:
    MyServerCallbacks(ArchStimV3 &dev) : device(dev) {}

    void onConnect(BLEServer *pServer)
    {
        device.deviceConnected = true;
        BLEDevice::setMTU(device.NEGOTIATE_MTU_SIZE);
        device.mtuSize = BLEDevice::getMTU() - device.MTU_HEADER_SIZE;

        // Connection indication
        digitalWrite(LED_B, HIGH);
        device.beep(2093, 100); // High C (C7)
    }

    void onDisconnect(BLEServer *pServer)
    {
        device.deviceConnected = false;

        // Disconnection indication
        digitalWrite(LED_B, LOW);
        device.beep(1047, 100); // Low C (C6)

        if (!device.continueOnDisconnect)
        {
            device.setActiveWaveform(nullptr);
            device.disableStim();
            device.deactivateIsolated();
        }

        device.continueOnDisconnect = false; // Reset flag for next connection

        // Restart advertising
        pServer->startAdvertising();
    }
};

// Command Characteristic Callbacks
class CommandCallbacks : public BLECharacteristicCallbacks
{
private:
    CommandInterpreter &cmdInterpreter;
    ArchStimV3 &device;

public:
    CommandCallbacks(CommandInterpreter &interpreter, ArchStimV3 &dev)
        : cmdInterpreter(interpreter), device(dev) {}

    void onWrite(BLECharacteristic *pCharacteristic)
    {
        String commands(pCharacteristic->getValue().c_str());
        if (commands.length() > 0)
        {
            int startPos = 0;
            int semicolonPos;

            while ((semicolonPos = commands.indexOf(';', startPos)) != -1)
            {
                String cmd = commands.substring(startPos, semicolonPos);
                cmd.trim();
                cmdInterpreter.processCommand(cmd);
                startPos = semicolonPos + 1;
            }

            if (startPos < commands.length())
            {
                String cmd = commands.substring(startPos);
                cmd.trim();
                cmdInterpreter.processCommand(cmd);
            }

            device.updateStatus();
        }
    }
};

void ArchStimV3::beginBLE(CommandInterpreter &interpreter)
{
    cmdInterpreter = &interpreter;

    // Initialize BLE
    String deviceName = "ARCH_";
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    char macStr[5];
    snprintf(macStr, sizeof(macStr), "%02X%02X", mac[4], mac[5]);
    deviceName += String(macStr);

    BLEDevice::init(deviceName.c_str());
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks(*this));

    BLEService *pService = pServer->createService(SERVICE_UUID);

    pStatusCharacteristic = pService->createCharacteristic(
        STATUS_CHAR_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

    pCommandCharacteristic = pService->createCharacteristic(
        COMMAND_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE);
    pCommandCharacteristic->setCallbacks(new CommandCallbacks(interpreter, *this));

    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
}

void ArchStimV3::updateStatus()
{
    if (!pStatusCharacteristic)
    {
        return; // Only check if characteristic exists
    }

    String status = "";

    // Running status
    status += "RUN:" + String(activeWaveform != nullptr ? 1 : 0) + ";";

    // Battery level (placeholder)
    status += "BAT:100;"; // Placeholder until battery monitoring is implemented

    // Impedance
    status += "Z:" + String(static_cast<int>(Z)) + ";";

    // SD card status
    status += "SD:" + String(SD.begin(SD_CS) ? 1 : 0) + ";";

    // USB connection status
    status += "USB:" + String(digitalRead(USB_SENSE) == HIGH ? 1 : 0) + ";";

    // Settings sync status
    status += "SYNC:1"; // Always synced for now

    pStatusCharacteristic->setValue(status.c_str());
    pStatusCharacteristic->notify();
}
