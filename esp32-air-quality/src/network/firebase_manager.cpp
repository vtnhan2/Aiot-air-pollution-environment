#include "firebase_manager.h"
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

// BẠN CẦN ĐIỀN THÔNG TIN FIREBASE CỦA BẠN VÀO ĐÂY:
#define API_KEY "AIzaSy_YOUR_API_KEY_HERE"
#define DATABASE_URL "https://your-project-id.firebaseio.com/" 

FirebaseManager firebaseManager;

void FirebaseManager::init() {
    Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

    // Gán API key và Database URL
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;

    // Đăng ký ẩn danh
    if (Firebase.signUp(&config, &auth, "", "")) {
        Serial.println("Firebase Auth OK");
        signupOK = true;
    } else {
        Serial.printf("%s\n", config.signer.signupError.message.c_str());
    }

    // Assign callbacks
    config.token_status_callback = tokenStatusCallback; 

    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
}

void FirebaseManager::loop() {
    // Không cần xử lý liên tục trong vòng lặp trừ khi bạn muốn listen data.
}

void FirebaseManager::sendData(float raw_pm25, float temp, float hum, float pres, float gas, float filtered_pm25, float predicted_pm25) {
    if (Firebase.ready() && signupOK) {
        // Tạo đường dẫn với Timestamp (giả lập hoặc dùng NTP)
        // Hiện tại ta chỉ lưu vào biến "current" để Dashboard đọc liên tục.
        
        // Gửi Raw Data
        Firebase.RTDB.setFloat(&fbdo, "sensor/raw_pm25", raw_pm25);
        Firebase.RTDB.setFloat(&fbdo, "sensor/temp", temp);
        Firebase.RTDB.setFloat(&fbdo, "sensor/hum", hum);
        Firebase.RTDB.setFloat(&fbdo, "sensor/pres", pres);
        Firebase.RTDB.setFloat(&fbdo, "sensor/gas", gas);
        
        // Gửi Data đã lọc và Dự báo AI
        Firebase.RTDB.setFloat(&fbdo, "ai/filtered_pm25", filtered_pm25);
        Firebase.RTDB.setFloat(&fbdo, "ai/predicted_pm25", predicted_pm25);
        
        Serial.println("Firebase: Data uploaded successfully!");
    } else {
        Serial.println("Firebase: Failed to upload data.");
    }
}
