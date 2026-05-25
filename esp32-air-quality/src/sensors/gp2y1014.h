#pragma once

#include <Arduino.h>

class GP2Y1014Sensor {
public:
    GP2Y1014Sensor(uint8_t measurePin, uint8_t ledPin);
    void begin();
    
    // Returns dust density in ug/m3
    float readDustDensity();

private:
    uint8_t _measurePin;
    uint8_t _ledPin;
    
    // Timing constants per datasheet
    const int samplingTime = 280;
    const int deltaTime = 40;
    const int sleepTime = 9680;
};
