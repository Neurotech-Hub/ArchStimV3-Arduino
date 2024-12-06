#ifndef COMMAND_INTERPRETER_H
#define COMMAND_INTERPRETER_H

#include "ArchStimV3.h"
#include "Waveforms/SquareWave.h"
#include "Waveforms/PulseWave.h"
#include "Waveforms/RandomPulseWave.h"
#include "Waveforms/SumOfSinesWave.h"
#include "Waveforms/RampedSineWave.h"

class CommandInterpreter
{
public:
    CommandInterpreter(ArchStimV3 &device) : device(device) {}

    void printHelp()
    {
        Serial.println("\nARCH Stim Commands:");
        Serial.println("System:");
        Serial.println("  EN            Enable stimulation");
        Serial.println("  DIS           Disable stimulation");
        Serial.println("  STP           Stop waveform");
        Serial.println("  BEP:f,d       Beep (freq in Hz, duration in ms)");
        Serial.println("  ZCK:c         Check impedance (channel 0-3)");
        Serial.println("  HELP          Show this help");
        Serial.println("  SETV:v        Set voltage (±4.096V)");
        Serial.println("  SETI:i        Set current (±2000µA)");
        Serial.println("  CONT:b        Continue stimulation after disconnect (0=off,1=on)");
        Serial.println("  STAT          Show device status");
        Serial.println("  TIME:y,m,d,h,m,s  Set RTC time (year,month,day,hour,min,sec)");
        Serial.println("\nWaveforms:");
        Serial.println("  SQR:n,p,f     Square (neg µA, pos µA, freq in Hz)");
        Serial.println("  PLS:a,b,c;t   Pulse (amp array in µA; time array in ms)");
        Serial.println("  RND:a,b,c     Random (amp array in µA)");
        Serial.println("  SOS:w0,f0,w1,f1,d  Sum of sines (weights in µA, freqs in Hz, duration in s)");
        Serial.println("  RMP:f,d,w,F,s   Ramped sine (rampFreq in Hz, dur in s, weight in µA, freq in Hz, step)");
        Serial.println("\nExamples:");
        Serial.println("  SQR:-500,500,10    // 10Hz square wave, ±500µA");
        Serial.println("  PLS:0,500,-500;100    // Pulse train, all steps 100ms");
        Serial.println("  PLS:0,500,-500;25,50,200  // Pulse train, different times per step");
        Serial.println("  RND:500,-500,250,-250  // Random pulses in µA");
        Serial.println("  BEP:1000,200       // 1kHz beep, 200ms\n");
    }

    bool processCommand(const String &command)
    {
        if (command == "HELP")
        {
            printHelp();
            return true;
        }

        // Validate command format
        if (command.length() == 0)
        {
            Serial.println("ERR: Empty command");
            return false;
        }

        // Split command into type and parameters
        int colonIndex = command.indexOf(':');
        String type = (colonIndex == -1) ? command : command.substring(0, colonIndex);
        String params = (colonIndex == -1) ? "" : command.substring(colonIndex + 1);

        if (type == "STP")
        {
            device.setAllCurrents(0);
            device.setActiveWaveform(nullptr);
            Serial.println("Waveform stopped");
            return true;
        }
        else if (type == "EN")
        {
            device.disableStim(); // ensure stim is disabled
            device.activateIsolated();
            device.enableStim();
            Serial.println("Stimulation enabled");
            return true;
        }
        else if (type == "DIS")
        {
            device.disableStim();
            device.deactivateIsolated();
            Serial.println("Stimulation disabled");
            return true;
        }
        else if (type == "BEP")
            return processBEP(params);
        else if (type == "ZCK")
        {
            int channel = 0; // Default to channel 0
            if (params.length() > 0)
            {
                channel = params.toInt();
                if (channel < 0 || channel > 3)
                {
                    Serial.println("ERR: Channel must be 0-3");
                    return false;
                }
            }
            device.zCheck(channel);
            return true;
        }
        else if (type == "SETV")
            return processSETV(params);
        else if (type == "SETI")
            return processSETI(params);
        else if (type == "CONT")
            return processCONT(params);
        else if (type == "SQR")
            return processSQR(params);
        else if (type == "PLS")
            return processPLS(params);
        else if (type == "RND")
            return processRND(params);
        else if (type == "SOS")
            return processSOS(params);
        else if (type == "RMP")
            return processRMP(params);
        else if (type == "STAT")
        {
            device.printStatus();
            return true;
        }
        else if (type == "TIME")
        {
            int values[6]; // year, month, day, hour, minute, second
            if (parseIntArray(params, values, 6) != 6)
            {
                Serial.println("ERR: TIME requires year,month,day,hour,minute,second");
                return false;
            }

            // Basic validation
            if (values[0] < 2000 || values[0] > 2099 || // year
                values[1] < 1 || values[1] > 12 ||      // month
                values[2] < 1 || values[2] > 31 ||      // day
                values[3] < 0 || values[3] > 23 ||      // hour
                values[4] < 0 || values[4] > 59 ||      // minute
                values[5] < 0 || values[5] > 59)        // second
            {
                Serial.println("ERR: Invalid time values");
                return false;
            }

            device.setTime(values[0], values[1], values[2], values[3], values[4], values[5]);
            device.updateTime(); // Show the current time after setting
            return true;
        }
        else
        {
            Serial.println("ERR: Unknown command type");
            return false;
        }
    }

private:
    ArchStimV3 &device;
    static const int MAX_ARRAY_SIZE = 10;
    static constexpr float MAX_FREQ = 1000.0;          // Maximum frequency in Hz
    static constexpr float MAX_DAC_VOLTAGE = 2 * VREF; // ±4.096V

    bool validateVoltage(float voltage)
    {
        if (voltage > device.V_COMPP || voltage < -device.V_COMPN)
        {
            Serial.println("ERR: Voltage exceeds V_COMP bounds.");
            return false;
        }
        return true;
    }

    bool validateFrequency(float freq)
    {
        if (freq <= 0 || freq > MAX_FREQ)
        {
            Serial.println("ERR: Invalid frequency");
            return false;
        }
        return true;
    }

    bool validateDuration(int duration)
    {
        if (duration < 1)
        {
            Serial.println("ERR: Duration must be positive");
            return false;
        }
        return true;
    }

    bool validateArraySize(int size)
    {
        if (size <= 0 || size > MAX_ARRAY_SIZE)
        {
            Serial.println("ERR: Invalid array size");
            return false;
        }
        return true;
    }

    bool validateDACVoltage(float voltage)
    {
        if (abs(voltage) > MAX_DAC_VOLTAGE)
        {
            Serial.println("ERR: Voltage exceeds ±4.096V limit");
            return false;
        }
        return true;
    }

    bool validateCurrent(int microAmps)
    {
        if (abs(microAmps) > MAX_CURRENT)
        {
            Serial.println("ERR: Current exceeds ±2000µA limit");
            return false;
        }
        return true;
    }

    bool processBEP(const String &params)
    {
        int values[2];
        if (parseIntArray(params, values, 2) != 2)
        {
            Serial.println("ERR: BEP requires frequency,duration");
            return false;
        }
        device.beep(values[0], values[1]);
        Serial.println("Beep played");
        return true;
    }

    bool processSQR(const String &params)
    {
        float values[3];
        if (parseFloatArray(params, values, 3) != 3)
        {
            Serial.println("ERR: SQR requires negVal,posVal,frequency");
            return false;
        }

        if (!validateCurrent(static_cast<int>(values[0])) ||
            !validateCurrent(static_cast<int>(values[1])) ||
            !validateFrequency(values[2]))
        {
            return false;
        }

        device.setActiveWaveform(
            new SquareWave(device, values[0], values[1], values[2]));
        Serial.println("Square wave started");
        return true;
    }

    bool processPLS(const String &params)
    {
        int splitIndex = params.indexOf(';');
        if (splitIndex == -1)
        {
            Serial.println("ERR: PLS requires ampArray;timeArray format");
            return false;
        }

        String ampStr = params.substring(0, splitIndex);
        String timeStr = params.substring(splitIndex + 1);

        int ampArray[MAX_ARRAY_SIZE];
        int timeArray[MAX_ARRAY_SIZE];

        int ampCount = parseIntArray(ampStr, ampArray, MAX_ARRAY_SIZE);
        if (!validateArraySize(ampCount))
            return false;

        int timeCount = parseIntArray(timeStr, timeArray, MAX_ARRAY_SIZE);
        if (timeCount != 1 && timeCount != ampCount)
        {
            Serial.println("ERR: Time array must be either single value or match amplitude array size");
            return false;
        }

        if (timeCount == 1)
        {
            int duration = timeArray[0];
            for (int i = 1; i < ampCount; i++)
            {
                timeArray[i] = duration;
            }
        }

        // Validate all currents (not voltages)
        for (int i = 0; i < ampCount; i++)
        {
            if (!validateCurrent(ampArray[i]))
                return false;
        }

        // Validate all durations
        for (int i = 0; i < timeCount; i++)
        {
            if (!validateDuration(timeArray[i]))
                return false;
        }

        device.setActiveWaveform(
            new PulseWave(device, ampArray, timeArray, ampCount));
        Serial.println("Pulse wave started");
        return true;
    }

    bool processRND(const String &params)
    {
        int ampArray[MAX_ARRAY_SIZE];
        int count = parseIntArray(params, ampArray, MAX_ARRAY_SIZE);

        if (!validateArraySize(count))
            return false;

        // Validate all currents (not voltages)
        for (int i = 0; i < count; i++)
        {
            if (!validateCurrent(ampArray[i]))
                return false;
        }

        device.setActiveWaveform(
            new RandomPulseWave(device, ampArray, count));
        Serial.println("Random pulse wave started");
        return true;
    }

    bool processSOS(const String &params)
    {
        float values[5]; // weight0, freq0, weight1, freq1, duration
        if (parseFloatArray(params, values, 5) != 5)
        {
            Serial.println("ERR: SOS requires weight0,freq0,weight1,freq1,duration");
            return false;
        }

        // Validate weights (currents, not voltages)
        if (!validateCurrent(static_cast<int>(values[0])) ||
            !validateCurrent(static_cast<int>(values[2])))
        {
            return false;
        }

        // Validate frequencies
        if (!validateFrequency(values[1]) || !validateFrequency(values[3]))
        {
            return false;
        }

        // Validate duration
        if (values[4] <= 0)
        {
            Serial.println("ERR: Duration must be positive");
            return false;
        }

        device.setActiveWaveform(
            new SumOfSinesWave(device, values[0], values[1], values[2], values[3], 1, values[4]));
        Serial.println("Sum of sines wave started");
        return true;
    }

    bool processRMP(const String &params)
    {
        float values[5]; // rampFreq, duration, weight0, freq0, stepSize
        if (parseFloatArray(params, values, 5) != 5)
        {
            Serial.println("ERR: RMP requires rampFreq,duration,weight,freq,step");
            return false;
        }

        // Validate frequency and current (not voltage)
        if (!validateFrequency(values[0]) || !validateFrequency(values[3]))
        {
            return false;
        }
        if (!validateCurrent(static_cast<int>(values[2])))
        {
            return false;
        }

        // Validate duration and step size
        if (values[1] <= 0 || values[4] <= 0)
        {
            Serial.println("ERR: Duration and step size must be positive");
            return false;
        }

        device.setActiveWaveform(
            new RampedSineWave(device, values[0], values[2], values[3], values[4], values[1]));
        Serial.println("Ramped sine wave started");
        return true;
    }

    bool processSETV(const String &params)
    {
        float voltage;
        if (parseFloatArray(params, &voltage, 1) != 1)
        {
            Serial.println("ERR: SETV requires voltage value");
            return false;
        }

        if (!validateDACVoltage(voltage))
        {
            return false;
        }

        device.setVoltage(voltage);
        Serial.print("Voltage set to: ");
        Serial.println(voltage);
        return true;
    }

    bool processSETI(const String &params)
    {
        int microAmps;
        if (parseIntArray(params, &microAmps, 1) != 1)
        {
            Serial.println("ERR: SETI requires current value in microamps");
            return false;
        }

        if (!validateCurrent(microAmps))
        {
            return false;
        }

        device.setAllCurrents(microAmps);
        Serial.print("Current set to: ");
        Serial.println(microAmps);
        return true;
    }

    bool processCONT(const String &params)
    {
        int value;
        if (parseIntArray(params, &value, 1) != 1)
        {
            Serial.println("ERR: CONT requires boolean value (0 or 1)");
            return false;
        }

        if (value != 0 && value != 1)
        {
            Serial.println("ERR: CONT value must be 0 or 1");
            return false;
        }

        device.continueOnDisconnect = (value == 1);
        Serial.print("after disconnect: ");
        Serial.println(value ? "ON" : "OFF");
        return true;
    }

    int parseFloatArray(const String &str, float *arr, int maxSize)
    {
        int count = 0;
        int start = 0;
        int comma;

        while ((comma = str.indexOf(',', start)) != -1 && count < maxSize)
        {
            String numStr = str.substring(start, comma);
            numStr.trim();
            if (numStr.length() == 0)
            {
                Serial.println("ERR: Empty value in array");
                return 0;
            }
            arr[count++] = numStr.toFloat();
            start = comma + 1;
        }

        if (start < str.length() && count < maxSize)
        {
            String numStr = str.substring(start);
            numStr.trim();
            if (numStr.length() == 0)
            {
                Serial.println("ERR: Empty value in array");
                return 0;
            }
            arr[count++] = numStr.toFloat();
        }

        return count;
    }

    int parseIntArray(const String &str, int *arr, int maxSize)
    {
        int count = 0;
        int start = 0;
        int comma;

        while ((comma = str.indexOf(',', start)) != -1 && count < maxSize)
        {
            String numStr = str.substring(start, comma);
            numStr.trim();
            if (numStr.length() == 0)
            {
                Serial.println("ERR: Empty value in array");
                return 0;
            }
            arr[count++] = numStr.toInt();
            start = comma + 1;
        }

        if (start < str.length() && count < maxSize)
        {
            String numStr = str.substring(start);
            numStr.trim();
            if (numStr.length() == 0)
            {
                Serial.println("ERR: Empty value in array");
                return 0;
            }
            arr[count++] = numStr.toInt();
        }

        return count;
    }
};

#endif
