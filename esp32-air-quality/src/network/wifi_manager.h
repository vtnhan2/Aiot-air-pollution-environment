#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>

class WiFiManager {
public:
    void init(const char* ssid, const char* password);
    void checkConnection();
    bool isConnected();

private:
    const char* _ssid;
    const char* _password;
    unsigned long _lastReconnectAttempt = 0;
};

extern WiFiManager wifiManager;

#endif // WIFI_MANAGER_H
