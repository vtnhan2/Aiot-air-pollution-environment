#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>

class AppWiFiManager {
public:
    void init();
    void checkConnection();
    bool isConnected();

private:
    unsigned long _lastReconnectAttempt = 0;
};

extern AppWiFiManager wifiManager;

#endif // WIFI_MANAGER_H
