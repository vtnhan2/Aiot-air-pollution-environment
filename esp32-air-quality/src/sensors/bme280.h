#pragma once

#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

class BME280Sensor {
public:
    BME280Sensor(uint8_t address = 0x76);
    bool begin(TwoWire *theWire = &Wire);
    
    float readTemperature();
    float readHumidity();
    float readPressure();

private:
    Adafruit_BME280 bme;
    uint8_t i2c_address;
};
