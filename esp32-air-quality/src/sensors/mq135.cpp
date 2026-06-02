#include "mq135.h"

MQ135Sensor::MQ135Sensor(uint8_t measurePin) : _measurePin(measurePin) {}

void MQ135Sensor::begin() {
    // Explicitly configure pin as input
    pinMode(_measurePin, INPUT);
    // Explicitly set read resolution and attenuation for ESP32-S3
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db); // Allow reading up to ~3.1V - 3.3V
}

float MQ135Sensor::readGasLevel() {
    int adcValue = analogRead(_measurePin);
    
    // Convert 12-bit ADC (0-4095) to Voltage (0-3.3V)
    float voltage = adcValue * (3.3 / 4095.0);
    
    // In a real application, you would calculate Rs/R0 and map to PPM 
    // using the MQ-135 datasheet curves for NH3, Benzene, etc.
    // For this AIoT edge node, sending the raw voltage or a normalized 0-100 scale 
    // is often better so the Cloud or AI model can process the raw trend.
    // Here we return the raw voltage as the "gas level" baseline.
    
    return voltage;
}
