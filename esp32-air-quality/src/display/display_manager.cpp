#include "display_manager.h"

// Định nghĩa mã màu HSL/RGB565 hiện đại
#define COLOR_BG        0x0842  // Xanh đen rất tối (Dark Navy)
#define COLOR_CARD      0x18C5  // Xanh đen xám (Glass card effect)
#define COLOR_TEXT_MUTED 0x9CB2 // Xám xanh nhạt
#define COLOR_WHITE     0xFFFF
#define COLOR_GREEN     0x3666  // Xanh lá mềm
#define COLOR_YELLOW    0xFCE0  // Vàng ấm
#define COLOR_RED       0xE204  // Đỏ neon
#define COLOR_CYAN      0x27FF  // Xanh ngọc
#define COLOR_MAGENTA   0xF19F  // Hồng tím (AI Accent)

DisplayManager::DisplayManager() : sprite(&tft) {}

void DisplayManager::begin() {
    tft.init();
    tft.setRotation(0); // 0, 1, 2, 3 tùy hướng xoay màn hình lắp thực tế
    
    // Khởi tạo Sprite 240x240 để vẽ đệm (Double-buffering)
    sprite.setColorDepth(16);
    
    // Allocate in PSRAM (vì ESP32-S3 N16R8 của chúng ta có 8MB PSRAM dư dả)
    if (psramFound()) {
        sprite.createSprite(240, 240);
        Serial.println("TFT Sprite created in PSRAM successfully.");
    } else {
        sprite.createSprite(240, 240);
        Serial.println("TFT Sprite created in internal RAM.");
    }
    
    // Xóa màn hình ban đầu
    tft.fillScreen(TFT_BLACK);
}

void DisplayManager::updateData(float temp, float hum, float raw_pm25, float filtered_pm25, float predicted_pm25, float gas_volts, bool wifi_connected, bool anomaly) {
    // 1. Clear Sprite bằng màu nền tối
    sprite.fillSprite(COLOR_BG);
    
    // 2. Vẽ khung viền trang trí bo góc nhẹ
    sprite.drawRoundRect(2, 2, 236, 236, 8, COLOR_CARD);
    
    // 3. Vẽ các phân vùng giao diện
    drawHeader(wifi_connected);
    drawPM25(filtered_pm25, raw_pm25, anomaly);
    drawAIPrediction(predicted_pm25);
    drawSubStats(temp, hum, gas_volts);
    
    // 4. Đẩy toàn bộ Sprite lên màn hình ST7789 cùng một lúc (Zero flicker)
    sprite.pushSprite(0, 0);
}

void DisplayManager::drawHeader(bool wifi_connected) {
    // Tiêu đề trạm đo
    sprite.setTextColor(COLOR_WHITE, COLOR_BG);
    sprite.drawString("AIoT AIR STATION", 10, 10, 2);
    
    // Wifi Badge
    if (wifi_connected) {
        sprite.fillRoundRect(170, 8, 60, 18, 4, COLOR_GREEN);
        sprite.setTextColor(COLOR_WHITE, COLOR_GREEN);
        sprite.drawCentreString("WiFi", 200, 10, 1);
    } else {
        sprite.fillRoundRect(170, 8, 60, 18, 4, COLOR_RED);
        sprite.setTextColor(COLOR_WHITE, COLOR_RED);
        sprite.drawCentreString("Offline", 200, 10, 1);
    }
    
    // Đường gạch ngang phân tách header
    sprite.drawLine(10, 32, 230, 32, COLOR_CARD);
}

void DisplayManager::drawPM25(float filtered, float raw, bool anomaly) {
    // Tiêu đề khu vực bụi
    sprite.setTextColor(COLOR_TEXT_MUTED, COLOR_BG);
    sprite.drawString("PM2.5 (Current)", 12, 38, 1);
    
    // Vẽ số liệu bụi (Dùng font cỡ lớn số 6 nếu là số nguyên, hoặc dùng font 4 để dễ canh lề)
    uint16_t color = getAQIColor(filtered);
    sprite.setTextColor(color, COLOR_BG);
    
    // Format chuỗi bụi mịn
    char pm_str[10];
    sprintf(pm_str, "%.1f", filtered);
    sprite.drawString(pm_str, 12, 48, 6); // Font 6: Chữ số lớn
    
    // Đơn vị ug/m3 nhỏ phía sau
    int text_width = sprite.textWidth(pm_str, 6);
    sprite.setTextColor(COLOR_TEXT_MUTED, COLOR_BG);
    sprite.drawString("ug/m3", 16 + text_width, 74, 2);
    
    // Hiển thị trạng thái/Cảnh báo Anomaly
    if (anomaly) {
        sprite.fillRoundRect(130, 48, 98, 38, 4, COLOR_RED);
        sprite.setTextColor(COLOR_WHITE, COLOR_RED);
        sprite.drawCentreString("SPIKE", 179, 52, 2);
        sprite.drawCentreString("FILTERED", 179, 68, 1);
    } else {
        String status = getAQIStatus(filtered);
        sprite.drawRoundRect(130, 48, 98, 38, 4, color);
        sprite.setTextColor(color, COLOR_BG);
        sprite.drawCentreString(status, 179, 58, 2);
    }
}

void DisplayManager::drawAIPrediction(float predicted) {
    // Card hiệu ứng Glassmorphism cho phần AI
    sprite.fillRoundRect(10, 98, 220, 68, 6, COLOR_CARD);
    
    // Label AI
    sprite.setTextColor(COLOR_MAGENTA, COLOR_CARD);
    sprite.drawString("🧠 AI FORECAST (Next 1h)", 18, 104, 2);
    
    // Giá trị dự báo
    sprite.setTextColor(COLOR_WHITE, COLOR_CARD);
    char pred_str[15];
    sprintf(pred_str, "%.1f ug/m3", predicted);
    sprite.drawString(pred_str, 18, 128, 4); // Font 4: Chữ trung bình lớn
    
    // Badge Đánh giá chất lượng không khí tương lai
    uint16_t color = getAQIColor(predicted);
    String status = getAQIStatus(predicted);
    
    sprite.fillRoundRect(150, 128, 70, 22, 4, color);
    sprite.setTextColor(COLOR_WHITE, color);
    sprite.drawCentreString(status, 185, 132, 1);
}

void DisplayManager::drawSubStats(float temp, float hum, float gas) {
    // Vẽ đường phân cách
    sprite.drawLine(10, 178, 230, 178, COLOR_CARD);
    
    // Chia làm 3 cột hiển thị: Nhiệt, Ẩm, Khí Gas
    
    // Cột 1: Nhiệt độ
    sprite.setTextColor(COLOR_TEXT_MUTED, COLOR_BG);
    sprite.drawCentreString("TEMP", 45, 184, 1);
    sprite.setTextColor(COLOR_CYAN, COLOR_BG);
    char temp_str[10];
    sprintf(temp_str, "%.1f C", temp);
    sprite.drawCentreString(temp_str, 45, 202, 2);
    
    // Cột 2: Độ ẩm
    sprite.setTextColor(COLOR_TEXT_MUTED, COLOR_BG);
    sprite.drawCentreString("HUMID", 120, 184, 1);
    sprite.setTextColor(COLOR_CYAN, COLOR_BG);
    char hum_str[10];
    sprintf(hum_str, "%.1f%%", hum);
    sprite.drawCentreString(hum_str, 120, 202, 2);
    
    // Cột 3: Khí Gas (MQ135)
    sprite.setTextColor(COLOR_TEXT_MUTED, COLOR_BG);
    sprite.drawCentreString("GAS VOLT", 195, 184, 1);
    
    // Đổi màu cảnh báo khí gas nếu điện áp cao
    if (gas > 1.8f) {
        sprite.setTextColor(COLOR_RED, COLOR_BG);
    } else {
        sprite.setTextColor(COLOR_WHITE, COLOR_BG);
    }
    char gas_str[10];
    sprintf(gas_str, "%.2f V", gas);
    sprite.drawCentreString(gas_str, 195, 202, 2);
}

uint16_t DisplayManager::getAQIColor(float pm25) {
    if (pm25 < 30.0f) return COLOR_GREEN;
    if (pm25 < 80.0f) return COLOR_YELLOW;
    return COLOR_RED;
}

String DisplayManager::getAQIStatus(float pm25) {
    if (pm25 < 30.0f) return "GOOD";
    if (pm25 < 80.0f) return "MODERATE";
    return "BAD";
}
