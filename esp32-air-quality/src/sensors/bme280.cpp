#include "bme280.h"

BME280Sensor::BME280Sensor(uint8_t address) : i2c_address(address) {}

bool BME280Sensor::begin(TwoWire *theWire) {
    if (!bme.begin(i2c_address, theWire)) {
        Serial.printf("Could not find BME280 at 0x%02X, trying alternative address...\n", i2c_address);
        uint8_t alt_address = (i2c_address == 0x76) ? 0x77 : 0x76;
        if (!bme.begin(alt_address, theWire)) {
            Serial.println("Could not find a valid BME280 sensor at both 0x76 and 0x77, check wiring!");
            return false;
        }
        i2c_address = alt_address;
        Serial.printf("BME280 found at alternative address: 0x%02X\n", i2c_address);
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
