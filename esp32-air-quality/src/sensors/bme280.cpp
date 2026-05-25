#include "bme280.h"

BME280Sensor::BME280Sensor(uint8_t address) : i2c_address(address) {}

bool BME280Sensor::begin(TwoWire *theWire) {
    if (!bme.begin(i2c_address, theWire)) {
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        return false;
    }
    
    // Default settings from datasheet
    bme.setSampling(Adafruit_BME280::MODE_NORMAL,
                    Adafruit_BME280::SAMPLING_X2,  // temp
                    Adafruit_BME280::SAMPLING_X16, // pressure
                    Adafruit_BME280::SAMPLING_X1,  // humidity
                    Adafruit_BME280::FILTER_X16,
                    Adafruit_BME280::STANDBY_MS_0_5);
    return true;
}

float BME280Sensor::readTemperature() {
    return bme.readTemperature();
}

float BME280Sensor::readHumidity() {
    return bme.readHumidity();
}

float BME280Sensor::readPressure() {
    return bme.readPressure() / 100.0F; // Return hPa
}
