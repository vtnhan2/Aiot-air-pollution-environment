#include "display_manager.h"

// Cần các thư viện này trong platformio.ini
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// Font
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSans12pt7b.h>

SPIClass* spiDisplay = nullptr;
Adafruit_ST7789* tft = nullptr;

DisplayManager::DisplayManager() {
    // Sẽ khởi tạo trong begin()
}

void DisplayManager::begin() {
    Serial.println("[DEBUG] DisplayManager::begin() started...");
    
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    
    // Khởi tạo SPI thủ công (SCLK=13, MISO=-1, MOSI=21, CS=10)
    spiDisplay = new SPIClass(FSPI);
    spiDisplay->begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
    tft = new Adafruit_ST7789(spiDisplay, TFT_CS, TFT_DC, TFT_RST);
    
    // init() dành cho màn 1.54 inch ST7789
    tft->init(240, 240);
    tft->setRotation(0);
    
    Serial.println("[DEBUG] ST7789 init done.");
    
    // Bài test màn hình và test đèn nền ngay khi khởi động:
    // Chớp đèn nền để kiểm tra xem LED bị đấu ngược (Active LOW/HIGH) không
    for (int i=0; i<3; i++) {
        digitalWrite(TFT_BL, HIGH);
        tft->fillScreen(RED);
        delay(300);
        digitalWrite(TFT_BL, LOW);
        tft->fillScreen(GREEN);
        delay(300);
    }
    digitalWrite(TFT_BL, HIGH); // Bật cố định
    
    tft->fillScreen(BLUE);
    delay(500);
    tft->fillScreen(BLACK);
    
    tft->setTextColor(WHITE);
    tft->setTextSize(2);
    tft->setCursor(20, 120);
    tft->print("SYSTEM BOOTING...");
    Serial.println("[DEBUG] Display ready.");
}

void DisplayManager::drawCentreString(const char* text, int x, int y, uint8_t size, uint16_t color) {
    tft->setTextSize(size);
    tft->setTextColor(color);
    int16_t x1, y1;
    uint16_t w, h;
    tft->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    tft->setCursor(x - w / 2, y + h / 2);
    tft->print(text);
}

uint16_t DisplayManager::getAQIColor(float pm25) {
    if (pm25 <= 12.0) return GREEN;
    if (pm25 <= 35.4) return YELLOW;
    if (pm25 <= 55.4) return ORANGE;
    if (pm25 <= 150.4) return RED;
    if (pm25 <= 250.4) return MAGENTA;
    return 0x780F; // Tím đậm / Nâu
}

String DisplayManager::getAQIStatus(float pm25) {
    if (pm25 <= 12.0) return "GOOD";
    if (pm25 <= 35.4) return "MODERATE";
    if (pm25 <= 55.4) return "SENSITIVE";
    if (pm25 <= 150.4) return "UNHEALTHY";
    if (pm25 <= 250.4) return "VERY UNHEALTHY";
    return "HAZARDOUS";
}

void DisplayManager::drawGrid() {
    tft->fillScreen(BLACK);
    
    // Viền ngoài
    tft->drawRect(0, 0, 240, 240, WHITE);
    
    // Khung PM2.5 (Nửa trái)
    tft->drawRect(0, 30, 120, 100, WHITE);
    
    // Khung Temp/Hum/Gas (Nửa phải)
    tft->drawRect(120, 30, 120, 100, WHITE);
    
    // Khung Dự báo AI (Dưới cùng)
    tft->drawRect(0, 130, 240, 110, WHITE);
}

void DisplayManager::drawHeader(bool wifi_connected) {
    tft->fillRect(0, 0, 240, 30, 0x01E8); // Xanh dương đậm
    tft->setFont(&FreeSans9pt7b);
    tft->setTextColor(WHITE);
    tft->setCursor(5, 20);
    tft->print("AIoT Monitor");
    
    // Icon WiFi
    if (wifi_connected) {
        tft->fillCircle(220, 15, 4, GREEN);
        tft->drawCircle(220, 15, 8, GREEN);
    } else {
        tft->drawLine(210, 5, 230, 25, RED);
        tft->drawLine(210, 25, 230, 5, RED);
    }
}

void DisplayManager::drawPM25(float filtered, float raw, bool anomaly) {
    uint16_t color = getAQIColor(filtered);
    
    tft->setFont(); // Font mặc định
    tft->setTextColor(WHITE);
    tft->setCursor(5, 40);
    tft->print("PM2.5 (ug/m3)");
    
    tft->setFont(&FreeSansBold12pt7b);
    tft->setTextColor(color);
    char pmBuf[10];
    sprintf(pmBuf, "%.1f", filtered);
    drawCentreString(pmBuf, 60, 80, 1, color);
    
    tft->setFont();
    tft->setTextColor(WHITE);
    String status = getAQIStatus(filtered);
    drawCentreString(status.c_str(), 60, 110, 1, color);
    
    if (anomaly) {
        tft->fillRoundRect(100, 35, 15, 15, 3, RED);
        tft->setTextColor(WHITE);
        tft->setCursor(104, 39);
        tft->print("!");
    }
}

void DisplayManager::drawSubStats(float temp, float hum, float gas) {
    tft->setFont(); // Font mặc định nhỏ
    tft->setTextColor(WHITE);
    
    // Nhiệt độ
    tft->setCursor(125, 45);
    tft->print("Temp: ");
    tft->setTextColor(YELLOW);
    tft->print(temp, 1);
    tft->print(" C");
    
    // Độ ẩm
    tft->setTextColor(WHITE);
    tft->setCursor(125, 75);
    tft->print("Hum:  ");
    tft->setTextColor(CYAN);
    tft->print(hum, 1);
    tft->print(" %");
    
    // Khí Gas
    tft->setTextColor(WHITE);
    tft->setCursor(125, 105);
    tft->print("Gas:  ");
    uint16_t gasColor = (gas > 1.5) ? RED : GREEN;
    tft->setTextColor(gasColor);
    tft->print(gas, 2);
    tft->print(" V");
}

void DisplayManager::drawAIPrediction(float predicted) {
    tft->fillRect(1, 131, 238, 25, 0x3186); // Nền xám cho Header AI
    tft->setFont(&FreeSans9pt7b);
    tft->setTextColor(WHITE);
    tft->setCursor(5, 149);
    tft->print("AI Forecast (1h)");
    
    uint16_t color = getAQIColor(predicted);
    
    tft->setFont(&FreeSansBold12pt7b);
    tft->setTextColor(color);
    char buf[10];
    sprintf(buf, "%.1f", predicted);
    drawCentreString(buf, 120, 190, 1, color);
    
    tft->setFont();
    tft->setTextColor(WHITE);
    String advice = "Normal";
    if (predicted > 50) advice = "Wear Mask!";
    if (predicted > 100) advice = "Stay Indoors!";
    
    drawCentreString(advice.c_str(), 120, 220, 1, color);
}

void DisplayManager::updateData(float temp, float hum, float raw_pm25, float filtered_pm25, float predicted_pm25, float gas_volts, bool wifi_connected, bool anomaly) {
    if (!tft) return;
    
    drawGrid();
    drawHeader(wifi_connected);
    drawPM25(filtered_pm25, raw_pm25, anomaly);
    drawSubStats(temp, hum, gas_volts);
    drawAIPrediction(predicted_pm25);
}

void DisplayManager::showWiFiSetup(String ssid, String pass) {
    if (!tft) return;
    tft->fillScreen(BLACK);
    
    tft->fillRoundRect(10, 10, 220, 40, 5, BLUE);
    tft->setTextColor(WHITE);
    tft->setFont();
    tft->setTextSize(2);
    tft->setCursor(30, 20);
    tft->print("WIFI SETUP");
    
    tft->setTextSize(1);
    tft->setTextColor(YELLOW);
    tft->setCursor(10, 80);
    tft->print("1. Ket noi WiFi vao mang:");
    
    tft->setTextColor(CYAN);
    tft->setCursor(10, 100);
    tft->print("SSID: ");
    tft->print(ssid);
    
    tft->setCursor(10, 120);
    tft->print("PASS: ");
    tft->print(pass);
    
    tft->setTextColor(WHITE);
    tft->setCursor(10, 160);
    tft->print("2. Vao trinh duyet nhap:");
    tft->setTextColor(GREEN);
    tft->setCursor(10, 180);
    tft->print("IP: 192.168.4.1");
}

void DisplayManager::showBootScreen() {
    if (!tft) return;
    tft->fillScreen(BLACK);
    
    tft->fillRoundRect(10, 10, 220, 40, 5, 0x03E0); // Dark Green
    tft->setTextColor(WHITE);
    tft->setFont();
    tft->setTextSize(2);
    tft->setCursor(25, 20);
    tft->print("SYSTEM READY");
    
    tft->setTextSize(1);
    tft->setTextColor(YELLOW);
    tft->setCursor(10, 80);
    tft->print("Xem bieu do Online tai:");
    
    tft->setTextColor(CYAN);
    tft->setTextSize(1);
    tft->setCursor(10, 110);
    tft->print("https://aiot-air-quality");
    tft->setCursor(10, 130);
    tft->print(".web.app");
    
    tft->setTextColor(WHITE);
    tft->setCursor(10, 200);
    tft->print("Dang bat dau thu thap...");
}

void DisplayManager::showConnectingScreen() {
    if (!tft) return;
    tft->fillScreen(BLACK);
    
    tft->fillRoundRect(10, 10, 220, 40, 5, 0xFD20); // Orange
    tft->setTextColor(WHITE);
    tft->setFont();
    tft->setTextSize(2);
    tft->setCursor(50, 20);
    tft->print("STARTING");
    
    tft->setTextSize(1);
    tft->setTextColor(YELLOW);
    tft->setCursor(10, 100);
    tft->print("Dang kiem tra mang WiFi...");
    
    tft->setTextColor(CYAN);
    tft->setCursor(10, 130);
    tft->print("Vui long doi giay lat.");
}
