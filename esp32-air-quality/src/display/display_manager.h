#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

class DisplayManager {
public:
    DisplayManager();
    void begin();
    
    // Cập nhật toàn bộ thông số lên màn hình
    void updateData(float temp, float hum, float raw_pm25, float filtered_pm25, float predicted_pm25, float gas_volts, bool wifi_connected, bool anomaly);

private:
    TFT_eSPI tft;
    TFT_eSprite sprite; // Sprite để vẽ nháp trước rồi đẩy lên màn hình (chống giật/flicker)
    
    // Hàm phụ trợ vẽ các thành phần giao diện
    void drawGrid();
    void drawHeader(bool wifi_connected);
    void drawPM25(float filtered, float raw, bool anomaly);
    void drawAIPrediction(float predicted);
    void drawSubStats(float temp, float hum, float gas);
    
    // Lấy màu sắc cảnh báo dựa trên nồng độ bụi
    uint16_t getAQIColor(float pm25);
    String getAQIStatus(float pm25);
};
