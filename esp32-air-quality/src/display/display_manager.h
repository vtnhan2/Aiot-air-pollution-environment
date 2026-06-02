#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Arduino.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>

// Các chân cắm (Đã Re-map sang GPIO có test xung 1Hz)
#define TFT_MOSI 39 // SDA
#define TFT_SCLK 40 // SCL
#define TFT_CS   41
#define TFT_DC   45
#define TFT_RST  48
#define TFT_BL   -1 // Không dùng chân cho BL, đấu cứng BL vào 3.3V

// Định nghĩa mã màu (RGB565)
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define ORANGE  0xFDA0

class DisplayManager {
public:
    DisplayManager();
    void begin();
    
    // Hàm cập nhật giao diện
    void updateData(float temp, float hum, float raw_pm25, float filtered_pm25, float predicted_pm25, float gas_volts, bool wifi_connected, bool anomaly);

private:
    void drawGrid();
    void drawHeader(bool wifi_connected);
    void drawPM25(float filtered, float raw, bool anomaly);
    void drawSubStats(float temp, float hum, float gas);
    void drawAIPrediction(float predicted);
    
    // Helpers
    uint16_t getAQIColor(float pm25);
    String getAQIStatus(float pm25);
    void drawCentreString(const char* text, int x, int y, uint8_t size, uint16_t color);
};

#endif
