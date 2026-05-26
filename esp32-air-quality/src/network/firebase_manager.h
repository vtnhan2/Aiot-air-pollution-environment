#ifndef FIREBASE_MANAGER_H
#define FIREBASE_MANAGER_H

#include <Firebase_ESP_Client.h>

class FirebaseManager {
public:
    void init();
    void loop();
    void sendData(float raw_pm25, float temp, float hum, float pres, float gas, float filtered_pm25, float predicted_pm25, bool pushToHistory = false);

private:
    FirebaseData fbdo;
    FirebaseAuth auth;
    FirebaseConfig config;
    
    unsigned long sendDataPrevMillis = 0;
    bool signupOK = false;
};

extern FirebaseManager firebaseManager;

#endif // FIREBASE_MANAGER_H
