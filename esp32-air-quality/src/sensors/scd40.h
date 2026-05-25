#pragma once

#include <Arduino.h>
#include <SensirionI2CScd4x.h>

class SCD40Sensor {
public:
    SCD40Sensor();
    bool begin(TwoWire *theWire = &Wire);
    
    bool readMeasurement(uint16_t &co2, float &temperature, float &humidity);

private:
    SensirionI2cScd4x scd4x;
    void printUint16Hex(uint16_t value);
};
