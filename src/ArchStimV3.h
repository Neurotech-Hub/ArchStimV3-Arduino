#ifndef ARCHSTIMV3_H
#define ARCHSTIMV3_H

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include "ADS1118.h"           // https://github.com/Neurotech-Hub/ADS1118-Arduino
#include "AD57X4R.h"           // https://github.com/Neurotech-Hub/AD57X4R-Arduino
#include "Adafruit_MAX1704X.h" // https://github.com/adafruit/Adafruit_MAX1704X
#include <PCF85263A.h>         // https://github.com/teddokano/RTC_NXP_Arduino, depends: https://github.com/Neurotech-Hub/I2C_device_Arduino
#include <time.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include "Waveforms/Waveform.h" // Base waveform class

// Define pins and constants as needed
#define USB_SENSE 1
#define USER_IN 2
#define SDA_PIN 3
#define SCL_PIN 4
#define LED_B 5
#define PS_HOLD 6
#define SMART_INT 7
#define EXT_OUTPUT 9
#define BUZZ 10
#define EXT_INPUT 11
#define ADC_CS 12
#define DAC_CS 13
#define LED_R 45
#define LED_STIM 46
#define MISO 37
#define SCK 36
#define MOSI 35
#define SD_CS 34
#define RTC_INT 39
#define DRIVE_EN 40
#define DISABLE 41
#define FUEL_ALERT 42

static constexpr double VREF = 2.048;
const int DAC_MIN = -32768;
const int DAC_MAX = 32767;
const int MAX_CURRENT = 2000;

// BLE configuration
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define STATUS_CHAR_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define COMMAND_CHAR_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a9"

class CommandInterpreter; // Forward declaration

class ArchStimV3
{
    // Forward declare the callback classes
    friend class MyServerCallbacks;
    friend class CommandCallbacks;

public:
    ArchStimV3();

    // ADC and DAC
    ADS1118 adc;
    AD57X4R dac;

    // hardware variables
    float V_COMPN = 32.0;
    float V_COMPP = 32.0;

    // public global variables
    float Z;
    // Initialization methods
    void begin();
    void initPins();
    void initSPI();
    void initI2C();
    void initSD();
    void initADC();
    void initDAC();

    // Waveform and pulse generation methods (called by specific waveform classes)
    void square(int negVal, int posVal, float frequency);
    void pulse(int ampArray[], int timeArray[], int arrSize);
    void randPulse(int ampArray[], int arrSize);
    void sumOfSines(int stepSize, float weight0, float freq0, float weight1, float freq1, int duration);
    void rampedSine(float rampFreq, float duration, float weight0, float freq0, int stepSize);

    // Waveform management
    void setActiveWaveform(Waveform *waveform); // Sets the current active waveform
    void runWaveform();                         // Executes the active waveform if set

    // getters and setters
    void activateIsolated();
    void deactivateIsolated();
    void enableStim();
    void disableStim();
    void beep(int frequency, int duration);
    void setRedLED();
    void zCheck(); // setter for Z
    void setCSDelay(uint16_t delay);

    // BLE methods
    void beginBLE(CommandInterpreter &cmdInterpreter);
    void updateStatus();
    bool isConnected() const { return deviceConnected; }
    void setConnected(bool connected) { deviceConnected = connected; }
    void updateMTUSize(uint16_t newSize) { mtuSize = newSize; }
    uint16_t getMTUSize() const { return mtuSize; }

    // BLE constants
    static constexpr uint16_t NEGOTIATE_MTU_SIZE = 515;
    static constexpr uint16_t MTU_HEADER_SIZE = 3;

    // Add this to the public section of the ArchStimV3 class
    void setAllCurrents(int microAmps); // Sets current for all channels (-2000 to 2000 µA)

    double getMilliVolts(uint8_t channel);
    void setVoltage(float voltage);
    uint16_t getRawADC(uint8_t channel);

    void handleUserButton(); // Called from ISR to handle button press

    // BLE user settings
    bool continueOnDisconnect = false;

    // Battery monitoring variables
    float batteryVoltage;
    float batteryPercent;

    // Battery monitoring methods
    void updateBatteryStatus();
    bool initBattery();

    // RTC instance
    PCF85263A rtc;

    // RTC methods
    bool initRTC();
    void updateTime();
    void setTime(int year, int month, int day, int hour, int minute, int second);

private:
    Waveform *activeWaveform; // Pointer to currently active waveform

    // BLE members
    BLEServer *pServer;
    BLECharacteristic *pStatusCharacteristic;
    BLECharacteristic *pCommandCharacteristic;
    bool deviceConnected;
    uint16_t mtuSize;

    // Store command interpreter reference
    CommandInterpreter *cmdInterpreter;

    // Button debounce variables
    static volatile unsigned long lastDebounceTime;
    static constexpr unsigned long debounceDelay = 250; // 250ms debounce time

    // Static method for ISR
    static void IRAM_ATTR userButtonISR();

    // Reference to instance for ISR
    static ArchStimV3 *instance;

    Adafruit_MAX17048 maxlipo; // Add battery monitor instance
};

#endif
