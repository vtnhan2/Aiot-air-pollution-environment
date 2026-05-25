#include <Arduino.h>
#include <WiFi.h>
#include "sensors/sensor_manager.h"

// WiFi Credentials
const char* ssid = "Ruby";
const char* password = "79797979";

SensorManager sensorManager;

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n--- AIoT Air Quality System Starting ---");
    
    // Connect to WiFi
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Check if PSRAM is initialized
    if (psramFound()) {
        Serial.printf("PSRAM initialized successfully. Size: %d bytes\n", ESP.getPsramSize());
    } else {
        Serial.println("Warning: PSRAM not found!");
    }

    sensorManager.begin();
    Serial.println("System setup complete.");
}

void loop() {
    SensorData data = sensorManager.readAll();
    
    Serial.println("\n--- Sensor Readings ---");
    Serial.printf("Temperature: %.2f °C\n", data.temperature);
    Serial.printf("Humidity: %.2f %%\n", data.humidity);
    Serial.printf("Pressure: %.2f hPa\n", data.pressure);
    Serial.printf("CO2: %d ppm\n", data.co2);
    Serial.printf("Dust (PM2.5 approx): %.2f ug/m3\n", data.dustDensity);
    Serial.printf("Gas Voltage (MQ135): %.2f V\n", data.gasVoltage);
    
    // Check WiFi status in loop
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi Status: Connected");
    } else {
        Serial.println("WiFi Status: Disconnected");
    }
    
    delay(5000); // Read every 5 seconds for now
}
