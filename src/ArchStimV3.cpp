#include "ArchStimV3.h"

ArchStimV3::ArchStimV3() : adc(ADC_CS), dac(DAC_CS, VREF) {}

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
    pinMode(EXT_OUTPUT, OUTPUT);
    pinMode(BUZZ, OUTPUT);
    pinMode(EXT_INPUT, INPUT);
    pinMode(ADC_CS, OUTPUT);
    pinMode(DAC_CS, OUTPUT);
    digitalWrite(ADC_CS, HIGH);
    digitalWrite(DAC_CS, HIGH);
    pinMode(LED_STIM, OUTPUT);
    pinMode(SD_CS, OUTPUT);
    pinMode(FUEL_ALERT, INPUT);
    pinMode(DISABLE, OUTPUT);
    pinMode(LED_B, OUTPUT);
    pinMode(DRIVE_EN, OUTPUT);
    digitalWrite(DRIVE_EN, LOW);

    digitalWrite(EXT_OUTPUT, LOW);
    digitalWrite(LED_R, LOW);
    digitalWrite(LED_STIM, LOW);
    digitalWrite(DISABLE, LOW);
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
    adc.setSamplingRate(adc::RATE_128SPS);
    adc.setInputSelected(adc::AIN_0);
    adc.setFullScaleRange(adc::FSR_4096);
}

void ArchStimV3::initDAC() {
    dac.setup(AD57X4R::AD5754R);
    dac.setAllOutputRanges(AD57X4R::BIPOLAR_5V);
    dac.setAllVoltages(0);
}

void ArchStimV3::square(float negVal, float posVal, float frequency) {
    static bool highState = false;
    static unsigned long lastToggleTime = 0;
    unsigned long interval = 1000000 / (2 * frequency);  // Half-period in microseconds

    unsigned long currentTime = micros();
    if (currentTime - lastToggleTime >= interval) {
        lastToggleTime = currentTime;
        highState = !highState;
        
        // Set DAC output based on state
        dac.setVoltage(highState ? posVal : negVal);
    }
}

void ArchStimV3::pulse(float ampArray[], int timeArray[], int arrSize) {
    long currentTime = millis();  // Initialize time value
    Z = zCheck();                 // Get head impedance (adjust as needed for your application)

    while (true) {
        for (int i = 0; i < arrSize; i++) {
            // Set DAC output directly using amplitude from ampArray, scaled as needed
            float amplitude = ampArray[i];
            float dacValue = min(DAC_MAX, max(DAC_MIN, amplitude));

            // Set DAC to calculated output level
            dac.setVoltage(dacValue);

            // Track timing for each pulse
            currentTime = millis();
            long targetTime = currentTime + timeArray[i];
            while (millis() < targetTime) {
                printCurrent();  // Keep updating or logging the current during the pulse period
            }
            // Check for "end" command to stop
        }
    }
}

void ArchStimV3::randPulse(float ampArray[], int arrSize) {
    Z = zCheck();  // Get impedance, if required

    while (true) {
        // Randomly select an amplitude from the array
        int randomIndex = random(0, arrSize);  // Randomly pick an index in the range
        float amplitude = ampArray[randomIndex];

        // Clamp the amplitude value between DAC_MIN and DAC_MAX
        float dacValue = min(DAC_MAX, max(DAC_MIN, amplitude));

        // Set DAC output
        dac.setVoltage(dacValue);

        // Set random duration for pulse between 50 ms and 500 ms
        int pulseDuration = random(50, 500);
        long startTime = millis();
        while (millis() - startTime < pulseDuration) {
            printCurrent();  // Continuously monitor or log during the pulse
        }

        // Check for "end" command to stop
    }
}

void ArchStimV3::readTimeSeries(float ampArray[], int arrSize, int stepSize) {
    for (int i = 0; i < arrSize; i++) {
        // Clamp the amplitude value between DAC_MIN and DAC_MAX
        float dacValue = min(DAC_MAX, max(DAC_MIN, ampArray[i]));

        // Set DAC output
        dac.setVoltage(dacValue);

        // Wait for the specified step size duration
        delay(stepSize);
    }
}

void ArchStimV3::sumOfSines(int stepSize, float weight0, float freq0, float weight1, float freq1, int duration) {
    unsigned long startTime = millis();
    unsigned long currentTime, nextStepTime;
    float currentSec;
    float amplitude;
    
    Z = zCheck();  // Find head impedance

    while (millis() - startTime < duration) {
        currentTime = millis();
        currentSec = currentTime * 0.001;  // Convert current time to seconds
        float mult = 2 * PI * currentSec;

        // Calculate amplitude based on sum of sines
        amplitude = weight0 * sin(mult * freq0) + weight1 * sin(mult * freq1);

        // Ensure amplitude is within DAC limits
        float dacValue = min(DAC_MAX, max(DAC_MIN, amplitude));

        // Set DAC output
        dac.setVoltage(dacValue);

        // Set timing for the next step
        nextStepTime = currentTime + stepSize;
        
        // Delay until the next step time
        while (millis() < nextStepTime) {
            printCurrent();  // Monitor or log current
        }
    }
}

void ArchStimV3::rampedSine(float rampFreq, float duration, float weight0, float freq0, int stepSize) {
    unsigned long startTime = millis();
    unsigned long currentTime;
    float currentSec;
    float startSec = startTime * 0.001;
    float amplitude;
    float zSetCurr = 0;
    float zSetR = 0;
    bool newZ = false;

    Z = zCheck();  // Get impedance (if needed)

    // Loop for the specified duration
    while (millis() - startTime < duration) {
        currentTime = millis();
        currentSec = currentTime * 0.001;  // Convert to seconds for frequency calculations
        float mult = 2 * PI * (currentSec - startSec);
        
        // Calculate amplitude using ramped sine and secondary sine with given frequencies
        amplitude = weight0 * sin(mult * rampFreq) + 0.5 * sin(mult * freq0);

        // Clamp amplitude within DAC range
        float dacValue = min(DAC_MAX, max(DAC_MIN, amplitude));

        // Set DAC output
        dac.setVoltage(dacValue);

        // Set timing for the next step
        unsigned long nextStepTime = currentTime + stepSize;
        
        // Delay until the next step time while monitoring current
        while (millis() < nextStepTime) {
            printCurrent();  // Log or monitor current

            // Example of tracking current changes if you need it for impedance updates
            if (abs(current) > 0.5) {  // Replace 'current' with actual current-reading logic
                zSetCurr = current;
                zSetR = dacValue;
                newZ = true;
            }
        }
    }

    // Experimental impedance calculation (modify as needed)
    if (newZ) {
        Z = (27.0 / (zSetCurr * 0.001)) - 1000.0 - (abs(zSetR * (200000.0 / DAC_MAX)));
        newZ = false;
    }
}

float ArchStimV3::zCheck() {
    return 0.0;
}

void ArchStimV3::printCurrent() {
    Serial.println("Current = ...");
}