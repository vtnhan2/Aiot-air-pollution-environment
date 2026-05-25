#pragma once

#include <Arduino.h>
#include "bme280.h"
#include "gp2y1014.h"
#include "mq135.h"

// Define GPIO pins based on implementation plan
#define PIN_I2C_SDA 8
#define PIN_I2C_SCL 9

#define PIN_GP2Y_MEASURE 4
#define PIN_GP2Y_LED 5

#define PIN_MQ135_MEASURE 1

struct SensorData {
    float temperature;
    float humidity;
    float pressure;
    float dustDensity;
    float gasVoltage;
    bool isValid;
};

class SensorManager {
public:
    SensorManager();
    void begin();
    SensorData readAll();

private:
    BME280Sensor bme;
    GP2Y1014Sensor dust;
    MQ135Sensor mq135;
};
