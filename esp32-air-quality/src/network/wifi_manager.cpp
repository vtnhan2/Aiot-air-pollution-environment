#include "wifi_manager.h"

WiFiManager wifiManager;

void WiFiManager::init(const char* ssid, const char* password) {
    _ssid = ssid;
    _password = password;
    
    Serial.printf("\nConnecting to WiFi: %s\n", _ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(_ssid, _password);
    
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 20) {
        delay(500);
        Serial.print(".");
        retries++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi Connected!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nFailed to connect to WiFi. Will retry in background.");
    }
}

void WiFiManager::checkConnection() {
    if (WiFi.status() != WL_CONNECTED) {
        unsigned long currentMillis = millis();
        // Try to reconnect every 10 seconds
        if (currentMillis - _lastReconnectAttempt >= 10000) {
            Serial.println("WiFi connection lost. Reconnecting...");
            WiFi.disconnect();
            WiFi.reconnect();
            _lastReconnectAttempt = currentMillis;
        }
    }
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}
