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
    initBattery();
    initRTC();

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
    adc.setFullScaleRange(ADS1118::FSR_2048);
    adc.setContinuousMode();
    adc.disablePullup();
    getMilliVolts(0); // dummy read to clear buffer
}

void ArchStimV3::initDAC()
{
    dac.setup(AD57X4R::AD5754R);
    dac.setAllOutputRanges(AD57X4R::BIPOLAR_5V);
    setAllCurrents(0);
}

bool ArchStimV3::initBattery()
{
    if (!maxlipo.begin())
    {
        Serial.println("MAX17048 not found!");
        return false;
    }

    Serial.println("MAX17048 found!");
    return true;
}

void ArchStimV3::updateBatteryStatus()
{
    float voltage = maxlipo.cellVoltage();
    if (!isnan(voltage))
    {
        batteryVoltage = voltage;
        batteryPercent = maxlipo.cellPercent();
    }
    else
    {
        batteryVoltage = 0.0;
        batteryPercent = 0.0;
    }
}

bool ArchStimV3::initRTC()
{
    if (rtc.oscillator_stop())
    {
        Serial.println("RTC oscillator stopped - needs time set");
        return false;
    }
    Serial.println("RTC initialized successfully");
    return true;
}

void ArchStimV3::updateTime()
{
    time_t current_time = rtc.time(NULL);
    Serial.print("Current time: ");
    Serial.println(ctime(&current_time));
}

void ArchStimV3::setTime(int year, int month, int day, int hour, int minute, int second)
{
    struct tm timeinfo;
    timeinfo.tm_year = year - 1900; // Years since 1900
    timeinfo.tm_mon = month - 1;    // 0-11
    timeinfo.tm_mday = day;         // 1-31
    timeinfo.tm_hour = hour;        // 0-23
    timeinfo.tm_min = minute;       // 0-59
    timeinfo.tm_sec = second;       // 0-59

    rtc.set(&timeinfo);
    Serial.println("RTC time set successfully");
}

void ArchStimV3::activateIsolated()
{
    digitalWrite(DRIVE_EN, HIGH);
    delay(200); // settle
    initDAC();  // sets to 0
    initADC();
    setRedLED();
}

void ArchStimV3::deactivateIsolated()
{
    setAllCurrents(0);
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

void ArchStimV3::setCSDelay(uint16_t delay)
{
    adc.csDelay = delay;
}

// helper function to get impedance from current
float ArchStimV3::getZ(int channel, int microAmps)
{
    double ADC = getMilliVolts(channel);
    float V = 0.0228 * ADC + -41.6177;
    float Z = V / (microAmps * 1e-6); // convert to ohms

    Serial.printf("Z Calculation Debug:\n");
    Serial.printf("  Channel: %d\n", channel);
    Serial.printf("  Current: %d µA\n", microAmps);
    Serial.printf("  ADC Reading: %.2f mV\n", ADC);
    Serial.printf("  Calculated V: %.4f V\n", V);
    Serial.printf("  Calculated Z: %.2f Ω\n", Z);
    Serial.println();

    return Z;
}

// helper function to set impedance
void ArchStimV3::setZ(float setZ)
{
    Z = setZ;
}

// uses Z_SWEEP to calculate the average impedance
void ArchStimV3::zCheck(int channel)
{
    Serial.printf("\n=== Starting Z Check on Channel %d ===\n", channel);
    float zSum = 0;
    for (int i = 0; i < sizeof(Z_SWEEP) / sizeof(Z_SWEEP[0]); i++)
    {
        int current = Z_SWEEP[i];
        Serial.printf("Step %d: Setting current to %d µA\n", i + 1, current);
        setAllCurrents(current);
        delay(50);

        float impedance = getZ(channel, current);
        zSum += impedance;
        Serial.printf("  Measured Z: %.2f Ω\n", impedance);
    }
    setAllCurrents(0);
    float avgZ = zSum / (sizeof(Z_SWEEP) / sizeof(Z_SWEEP[0]));
    setZ(avgZ);
    Serial.printf("=== Z Check Complete: Average Z = %.2f Ω ===\n\n", avgZ);
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
        return;
    }

    // Update battery status before sending
    updateBatteryStatus();

    String status = "";
    status += "RUN:" + String(activeWaveform != nullptr ? 1 : 0) + ";";

    // Use actual battery percentage
    status += "BAT:" + String(static_cast<int>(batteryPercent)) + ";";

    // Impedance
    status += "Z:" + String(static_cast<int>(Z)) + ";";

    // SD card status
    status += "SD:" + String(SD.begin(SD_CS) ? 1 : 0) + ";";

    // USB connection status
    status += "USB:" + String(digitalRead(USB_SENSE) == HIGH ? 1 : 0) + ";";

    // Settings sync status
    status += "SYNC:1"; // Always synced for now

    Serial.println(status);

    pStatusCharacteristic->setValue(status.c_str());
    pStatusCharacteristic->notify();
}

// Runs the active waveform if one is set
void ArchStimV3::runWaveform()
{
    if (activeWaveform)
    {
        // Check timeout if enabled
        if (stimTimeout > 0)
        {
            unsigned long currentTime = millis();
            if (currentTime - stimStartTime >= stimTimeout)
            {
                // Stop the waveform
                setAllCurrents(0);
                delete activeWaveform;
                activeWaveform = nullptr;
                stimTimeout = 0; // Reset timeout
                Serial.println("Stimulation stopped due to timeout");
                return;
            }
        }

        activeWaveform->execute();
    }
}

// Generates a square wave with specified negative and positive currents at given frequency
// @param negVal: negative current value (µA)
// @param posVal: positive current value (µA)
// @param frequency: wave frequency (Hz)
// Example: square(-2000, 2000, 10.0) // ±2000µA square wave at 10Hz
//
// Square Wave Pattern:
//
// Time:      0ms    50ms   100ms  150ms  200ms
//           |      |      |      |      |
// Current:   2000µA -2000µA 2000µA -2000µA 2000µA
//           ┌──────┐      ┌──────┐      ┌────
//           │      │      │      │      │
//           │      │      │      │      │
//           │      └──────┘      └──────┘
//          -2000µA
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
    static bool initialized = false;
    static int savedNegVal = 0;
    static int savedPosVal = 0;
    static float savedFreq = 0;

    // Reset state if reset is needed or if parameters change
    if (isWaveformResetNeeded() || !initialized || savedNegVal != negVal || savedPosVal != posVal || savedFreq != frequency)
    {
        highState = false;
        lastToggleTime = micros();
        savedNegVal = negVal;
        savedPosVal = posVal;
        savedFreq = frequency;
        initialized = true;
    }

    unsigned long interval = 1000000 / (2 * frequency); // Half-period in microseconds
    unsigned long currentTime = micros();

    if (currentTime - lastToggleTime >= interval)
    {
        lastToggleTime = currentTime;
        highState = !highState;
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

    // Reset if needed
    if (isWaveformResetNeeded())
    {
        currentIndex = 0;
        lastTransitionTime = millis();
    }

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

    // Reset if needed
    if (isWaveformResetNeeded())
    {
        inZeroState = true;
        lastTransitionTime = millis();
        currentDuration = 1000 + random(501);
        currentAmplitude = 0;
        setAllCurrents(0);
    }

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

    // Reset if needed
    if (isWaveformResetNeeded())
    {
        startTime = millis();
        lastStepTime = 0;
    }

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
//    1000µA ─┤ └──┐                       ┌──┘ ├── 1000µA
//            │    │                       │    │
//       0µA ─┼────┼───────────────────────┼────┼─── 0µA
//            │    │                       │    │
//   -1000µA ─┤ ┌──┘                       └── ├─── -1000µA
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

    // Reset if needed
    if (isWaveformResetNeeded())
    {
        startTime = millis();
        lastStepTime = 0;
    }

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

void ArchStimV3::printStatus()
{
    String divider = "├───────────────┼────────────────────────────────┤";
    Serial.println("\n┌───────────────┬────────────────────────────────┐");
    Serial.println("│ Parameter     │ Value                          │");
    Serial.println(divider);

    // Format each status line with consistent width
    Serial.printf("│ Stimulation  │ %s\n", activeWaveform ? "RUNNING" : "STOPPED");
    Serial.printf("│ Battery      │ %.1f%% (%.2fV)\n", batteryPercent, batteryVoltage);
    Serial.printf("│ Impedance    │ %.0f Ω\n", Z);
    Serial.printf("│ SD Card      │ %s\n", SD.begin(SD_CS) ? "CONNECTED" : "NOT FOUND");
    Serial.printf("│ USB          │ %s\n", digitalRead(USB_SENSE) == HIGH ? "CONNECTED" : "DISCONNECTED");
    Serial.printf("│ Drive        │ %s\n", digitalRead(DRIVE_EN) == HIGH ? "ENABLED" : "DISABLED");
    Serial.printf("│ Stimulator   │ %s\n", digitalRead(DISABLE) == LOW ? "ENABLED" : "DISABLED");

    Serial.println(divider);
    Serial.println();
}

// Generates a pure sine wave with specified amplitude and frequency
// @param amplitude: peak amplitude of the sine wave (µA)
// @param frequency: wave frequency (Hz)
// Example: sine(500, 10.0) // 500µA sine wave at 10Hz
//
// Sine Wave Pattern:
//
// Time:      0ms    25ms   50ms   75ms   100ms
//            |      |      |      |      |
// Current:   500µA                             500µA
//            ┌──────────────────────────────┐
//            │      Pure Sine Wave          │
//            │                              │
//       0µA ─┼──────────────────────────────┼─── 0µA
//            │                              │
//            │                              │
//    -500µA  └──────────────────────────────┘    -500µA
//
// Details:
// Period:    100ms (10Hz)
// Phase:     Starts at 0
// Range:     ±amplitude µA
//
void ArchStimV3::sine(int amplitude, float frequency)
{
    static unsigned long sineStartTime = 0;

    // Reset if needed
    if (isWaveformResetNeeded())
    {
        sineStartTime = micros();
    }

    unsigned long currentTime = micros();
    float t = (currentTime - sineStartTime) / 1000000.0f; // Convert to seconds, relative to start time

    // Calculate sine wave value (starts from 0 phase)
    float value = amplitude * sin(2 * PI * frequency * t);

    // Update the output
    setAllCurrents(static_cast<int>(value));
}