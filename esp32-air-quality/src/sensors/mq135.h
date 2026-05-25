#pragma once

#include <Arduino.h>

class MQ135Sensor {
public:
    MQ135Sensor(uint8_t measurePin);
    void begin();
    
    // Returns estimated AQI / Gas concentration based on voltage
    float readGasLevel();

private:
    uint8_t _measurePin;
};
