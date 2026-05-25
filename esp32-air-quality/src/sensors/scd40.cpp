#include "scd40.h"

SCD40Sensor::SCD40Sensor() {}

bool SCD40Sensor::begin(TwoWire *theWire) {
    scd4x.begin(*theWire, 0x62);

    uint16_t error;
    char errorMessage[256];

    // Stop potentially previously started measurement
    error = scd4x.stopPeriodicMeasurement();
    if (error) {
        Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }

    // Start Measurement
    error = scd4x.startPeriodicMeasurement();
    if (error) {
        Serial.print("Error trying to execute startPeriodicMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
        return false;
    }
    
    return true;
}

bool SCD40Sensor::readMeasurement(uint16_t &co2, float &temperature, float &humidity) {
    uint16_t error;
    char errorMessage[256];
    bool isDataReady = false;
    
    error = scd4x.getDataReadyStatus(isDataReady);
    if (error) {
        return false;
    }
    
    if (!isDataReady) {
        return false;
    }
    
    error = scd4x.readMeasurement(co2, temperature, humidity);
    if (error) {
        Serial.print("Error trying to execute readMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
        return false;
    } else if (co2 == 0) {
        Serial.println("Invalid sample detected, skipping.");
        return false;
    }
    
    return true;
}
