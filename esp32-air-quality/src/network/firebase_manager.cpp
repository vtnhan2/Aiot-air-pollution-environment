#include "firebase_manager.h"
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

#include "config.h"
#define API_KEY FIREBASE_API_KEY
#define DATABASE_URL FIREBASE_DATABASE_URL

FirebaseManager firebaseManager;

void FirebaseManager::init() {
    Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

    // Gán API key và Database URL
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;

    Serial.println("[DEBUG] FirebaseManager: calling signUp...");
    // Đăng ký ẩn danh
    if (Firebase.signUp(&config, &auth, "", "")) {
        Serial.println("Firebase Auth OK");
        signupOK = true;
    } else {
        Serial.printf("%s\n", config.signer.signupError.message.c_str());
    }

    // Assign callbacks
    config.token_status_callback = tokenStatusCallback; 

    Serial.println("[DEBUG] FirebaseManager: calling begin...");
    Firebase.begin(&config, &auth);
    Serial.println("[DEBUG] FirebaseManager: calling reconnectWiFi...");
    Firebase.reconnectWiFi(true);
    Serial.println("[DEBUG] FirebaseManager::init() complete.");
}

void FirebaseManager::loop() {
    // Không cần xử lý liên tục trong vòng lặp trừ khi bạn muốn listen data.
}

void FirebaseManager::sendData(float raw_pm25, float temp, float hum, float pres, float gas, float filtered_pm25, float predicted_pm25, bool pushToHistory) {
    if (Firebase.ready() && signupOK) {
        // Gửi Raw Data
        Firebase.RTDB.setFloat(&fbdo, "sensor/raw_pm25", raw_pm25);
        Firebase.RTDB.setFloat(&fbdo, "sensor/temp", temp);
        Firebase.RTDB.setFloat(&fbdo, "sensor/hum", hum);
        Firebase.RTDB.setFloat(&fbdo, "sensor/pres", pres);
        Firebase.RTDB.setFloat(&fbdo, "sensor/gas", gas);
        
        // Gửi Data đã lọc và Dự báo AI
        Firebase.RTDB.setFloat(&fbdo, "ai/filtered_pm25", filtered_pm25);
        Firebase.RTDB.setFloat(&fbdo, "ai/predicted_pm25", predicted_pm25);
        
        Serial.println("Firebase: Current data updated!");

        // Lưu lịch sử nếu được yêu cầu (ví dụ: định kỳ mỗi 1 phút)
        if (pushToHistory) {
            FirebaseJson json;
            json.add("raw_pm25", raw_pm25);
            json.add("temp", temp);
            json.add("hum", hum);
            json.add("pres", pres);
            json.add("gas", gas);
            json.add("filtered_pm25", filtered_pm25);
            json.add("predicted_pm25", predicted_pm25);
            
            // Server-side timestamp
            FirebaseJson ts;
            ts.add(".sv", "timestamp");
            json.add("timestamp", ts);
            
            if (Firebase.RTDB.pushJSON(&fbdo, "/history", &json)) {
                Serial.println("Firebase: History log appended successfully!");
            } else {
                Serial.printf("Firebase: Failed to append history. Error: %s\n", fbdo.errorReason().c_str());
            }
        }
    } else {
        Serial.println("Firebase: Failed to upload data.");
    }
}
