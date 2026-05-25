#include "gp2y1014.h"

GP2Y1014Sensor::GP2Y1014Sensor(uint8_t measurePin, uint8_t ledPin) 
    : _measurePin(measurePin), _ledPin(ledPin) {}

void GP2Y1014Sensor::begin() {
    pinMode(_ledPin, OUTPUT);
    // ADC configuration should be done in main setup or here,
    // usually Arduino handles analogRead config automatically on first call
    // ESP32 ADC resolution is 12-bit by default (0-4095) for 0-3.3V
    // Note: The GP2Y1014AU requires 5V for V-LED and VCC. 
    // The Vo output can go up to ~3.5V, so ESP32 ADC might need attenuation config if needed, 
    // but default 11dB attenuation handles up to ~3.1V.
}

float GP2Y1014Sensor::readDustDensity() {
    digitalWrite(_ledPin, LOW); // Power on the LED
    delayMicroseconds(samplingTime);

    // Read the dust value via analog pin
    int voMeasured = analogRead(_measurePin);
    
    delayMicroseconds(deltaTime);
    digitalWrite(_ledPin, HIGH); // Turn the LED off
    delayMicroseconds(sleepTime);

    // Convert analog value to Voltage (assuming 3.3V / 4095 resolution)
    // ESP32 default ADC attenuation is 11dB (~150mV - ~3100mV max reading)
    // For a more precise voltage, we use the standard conversion
    float calcVoltage = voMeasured * (3.3 / 4095.0);

    // Linear equation taken from Chris Nafis (http://www.howmuchsnow.com/arduino/airquality/)
    // Dust Density [ug/m3] = (calcVoltage - noDustVoltage) / 0.005
    // Typical noDustVoltage is ~0.6V (can vary by sensor)
    float dustDensity = 0.17 * calcVoltage - 0.1;
    
    if (dustDensity < 0) {
        dustDensity = 0.0;
    }
    
    return dustDensity * 1000.0; // convert mg/m3 to ug/m3
}
