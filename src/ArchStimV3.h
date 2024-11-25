#ifndef ARCHSTIMV3_H
#define ARCHSTIMV3_H

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include "ADS1118.h"
#include "AD57X4R.h"
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

const double VREF = 2.048;
const int DAC_MIN = -32768;
const int DAC_MAX = 32767;

class ArchStimV3
{
public:
    ArchStimV3();

    // Initialization methods
    void begin();
    void initPins();
    void initSPI();
    void initI2C();
    void initADC();
    void initDAC();

    // Waveform and pulse generation methods (called by specific waveform classes)
    void square(float negVal, float posVal, float frequency);
    void pulse(float ampArray[], int timeArray[], int arrSize);
    void randPulse(float ampArray[], int arrSize);
    void readTimeSeries(float ampArray[], int arrSize, int stepSize);
    void sumOfSines(int stepSize, float weight0, float freq0, float weight1, float freq1, int duration);
    void rampedSine(float rampFreq, float duration, float weight0, float freq0, int stepSize);

    // Utility functions
    float zCheck();
    void printCurrent();
    void setCSDelay(uint16_t delay);
    // Waveform management
    void setActiveWaveform(Waveform *waveform); // Sets the current active waveform
    void runWaveform();                         // Executes the active waveform if set

    // getters and setters
    void activateIsolated();
    void deactivateIsolated();
    void enableStim();
    void disableStim();
    void beep(int frequency, int duration);

    double getMilliVolts(uint8_t channel);
    void setVoltage(float voltage);
    uint16_t getRawADC(uint8_t channel);

private:
    ADS1118 adc;
    AD57X4R dac;
    float Z; // Head impedance or similar variable, initialized in functions as needed

    Waveform *activeWaveform; // Pointer to currently active waveform
};

#endif
