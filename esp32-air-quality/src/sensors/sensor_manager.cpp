#include "sensor_manager.h"
#include <Wire.h>

SensorManager::SensorManager() 
    : dust(PIN_GP2Y_MEASURE, PIN_GP2Y_LED), mq135(PIN_MQ135_MEASURE) {}

void SensorManager::begin() {
    Serial.println("Initializing I2C...");
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    delay(500);
    
    Serial.println("Initializing BME280...");
    if (!bme.begin(&Wire)) {
        Serial.println("Failed to init BME280!");
    }
    delay(500);
    
    Serial.println("Initializing SCD40...");
    if (!scd40.begin(&Wire)) {
        Serial.println("Failed to init SCD40!");
    }
    delay(500);
    
    Serial.println("Initializing GP2Y1014 Dust Sensor...");
    dust.begin();
    delay(500);
    
    Serial.println("Initializing MQ135...");
    mq135.begin();
    delay(500);
    Serial.println("Sensor initialization finished.");
}

SensorData SensorManager::readAll() {
    SensorData data;
    data.isValid = true;
    
    // Read BME280
    data.temperature = bme.readTemperature();
    data.humidity = bme.readHumidity();
    data.pressure = bme.readPressure();
    
    // Read SCD40 (Overrides temp/hum if successful since it's inside)
    float scdTemp = 0, scdHum = 0;
    uint16_t co2 = 0;
    if (scd40.readMeasurement(co2, scdTemp, scdHum)) {
        data.co2 = co2;
        // You can choose to average BME and SCD temps or pick one
        // Let's use SCD40 for temp/hum as it's highly accurate photoacoustic
        data.temperature = scdTemp;
        data.humidity = scdHum;
    } else {
        data.co2 = 0; // 0 indicates invalid read
    }
    
    // Read Dust & Gas
    data.dustDensity = dust.readDustDensity();
    data.gasVoltage = mq135.readGasLevel();
    
    return data;
}
