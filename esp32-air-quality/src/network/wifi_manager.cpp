#include "wifi_manager.h"
#include <WiFiManager.h> // tzapu WiFiManager

#define PIN_BOOT_BUTTON 0 // Nút BOOT trên kit ESP32-S3

AppWiFiManager wifiManager;

#include "../display/display_manager.h"
extern DisplayManager displayManager;

// Callback tự động gọi khi WiFiManager không kết nối được WiFi đã lưu và bắt đầu phát AP Setup
void configModeCallback(::WiFiManager *myWiFiManager) {
    String ssid = myWiFiManager->getConfigPortalSSID();
    String pass = "12345678";
    
    Serial.println("\n==================================================");
    Serial.println("[WiFi-DEBUG] LẦN ĐẦU KHỞI ĐỘNG HOẶC KHÔNG TÌM THẤY WIFI ĐÃ LƯU!");
    Serial.printf("[WiFi-DEBUG] Đang phát Access Point Setup: %s (Pass: %s)\n", ssid.c_str(), pass.c_str());
    Serial.println("[WiFi-DEBUG] Vui lòng lấy điện thoại/máy tính kết nối vào WiFi này");
    Serial.println("[WiFi-DEBUG] để cấu hình mạng WiFi mới.");
    Serial.println("==================================================\n");
    
    // Hiển thị lên màn hình TFT
    displayManager.showWiFiSetup(ssid, pass);
}

void AppWiFiManager::init() {
    ::WiFiManager wm;

    // Đăng ký callback khi vào chế độ cấu hình phát AP
    wm.setAPCallback(configModeCallback);

    // Timeout cấu hình: Sau 3 phút không thao tác sẽ tự động tắt AP cấu hình để chạy offline
    wm.setConfigPortalTimeout(180);

    // Kiểm tra xem nút BOOT có được giữ khi khởi động không để xóa sạch cấu hình WiFi cũ
    pinMode(PIN_BOOT_BUTTON, INPUT_PULLUP);
    Serial.print("[WiFi] Đang khởi chạy. Nhấn giữ nút BOOT (GPIO 0) trong 3 giây tới để Reset WiFi: ");
    bool forceReset = false;
    for (int i = 0; i < 30; i++) { // 30 * 100ms = 3 giây
        if (digitalRead(PIN_BOOT_BUTTON) == LOW) {
            forceReset = true;
            break;
        }
        delay(100);
        Serial.print(".");
    }
    Serial.println();

    if (forceReset) {
        Serial.println("\n[WiFi] Đã nhận tín hiệu nút BOOT! Đang xóa cấu hình cũ...");
        wm.resetSettings();
        delay(1000);
    }

    Serial.println("\n[WiFi] Đang tìm kiếm và kết nối mạng đã lưu...");
    displayManager.showConnectingScreen(); // Hiển thị màn hình chờ kết nối
    
    // Tự động kết nối tới WiFi cũ. Nếu không thấy hoặc thất bại, phát AP cấu hình:
    // Tên mạng AP: "AirQuality_AIoT_Setup", Mật khẩu: "12345678"
    bool success = wm.autoConnect("AirQuality_AIoT_Setup", "12345678");

    if (!success) {
        Serial.println("[WiFi] Kết nối thất bại hoặc quá thời gian cấu hình. Chạy ở chế độ offline/giả lập.");
    } else {
        Serial.println("[WiFi] Kết nối WiFi thành công!");
        Serial.print("[WiFi] Địa chỉ IP: ");
        Serial.println(WiFi.localIP());
    }
}

void AppWiFiManager::checkConnection() {
    if (WiFi.status() != WL_CONNECTED) {
        unsigned long currentMillis = millis();
        // Tự động kết nối lại mỗi 10 giây nếu mất kết nối
        if (currentMillis - _lastReconnectAttempt >= 10000) {
            Serial.println("[WiFi] Mất kết nối mạng. Đang kết nối lại...");
            WiFi.disconnect();
            WiFi.reconnect();
            _lastReconnectAttempt = currentMillis;
        }
    }
}

bool AppWiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}
