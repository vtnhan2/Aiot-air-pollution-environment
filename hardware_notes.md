# Hướng dẫn Đấu nối Phần cứng & Sơ đồ chân (Wiring Pinout)
**Dự án:** Hệ thống giám sát và Dự báo chất lượng không khí (AIoT Air Quality)
**Vi điều khiển:** ESP32-S3-DevKitC-1 N16R8

Dưới đây là sơ đồ đấu nối chi tiết giữa ESP32-S3 và các linh kiện trong danh sách chốt của bạn.

---

## 1. Sơ đồ phân bổ chân (Pinout Mapping)

| Linh kiện | Chân trên Linh kiện | Chân trên ESP32-S3 | Ghi chú |
| :--- | :--- | :--- | :--- |
| **Cảm biến BME280** | VCC | 3.3V | Cấp nguồn 3.3V |
| | GND | GND | Ground |
| | SDA | **GPIO 21** | Giao tiếp I2C |
| | SCL | **GPIO 9** | Giao tiếp I2C |
| **Màn hình ST7789 IPS** | VCC | 3.3V | Cấp nguồn 3.3V |
| | GND | GND | Ground |
| | MOSI (SDA) | **GPIO 39** | Giao tiếp SPI Data |
| | SCLK (SCL) | **GPIO 40** | Giao tiếp SPI Clock |
| | CS | **GPIO 41** | Chip Select |
| | DC (RS/Data-Cmd)| **GPIO 45** | Chân chọn Dữ liệu/Lệnh |
| | RES (Reset) | **GPIO 48** | Chân reset màn hình |
| | BLK (Backlight) | **3.3V** | Nối thẳng 3.3V để luôn sáng |
| **Cảm biến Bụi GP2Y1014** | VCC (Chân 1) | 5V (Nguồn ngoài 5V) | Cần mạch lọc RC |
| | GND (Chân 2 & 4) | GND | Nối chung Ground |
| | LED (Chân 3) | **GPIO 17** | Chân xuất tín hiệu kích LED |
| | Vo (Chân 5) | **GPIO 4** | Chân đọc tín hiệu Analog (ADC1) |
| **Cảm biến MQ135** | VCC | 5V (Nguồn ngoài) | Sấy nóng tốn điện |
| | GND | GND | Ground |
| | AO (Analog Out) | **GPIO 16** | Chân đọc tín hiệu Analog (ADC2) |
| **Còi Buzzer + LED Báo động** | Cực (+) | **GPIO 18** | Dùng chung 1 chân cho cả còi và còi báo động. Đệm thêm trở 220 Ohm cho LED. |
| | GND (-) | GND | Ground |

---

## 2. Sơ đồ đấu nối chi tiết Mạch lọc RC cho GP2Y1014
Cảm biến Sharp GP2Y1014 bắt buộc phải nối qua 1 tụ điện 220uF và trở 150 Ohm để bảo vệ bóng LED phát hồng ngoại bên trong:

```text
               [ Trở 150 Ohm ]
5V (Nguồn) ----[======]-------- Chân 1 (VCC) cảm biến Sharp
                         |
                       ===== Tụ Hóa 220uF (Chân dài/Dương)
                         |
GND ---------------------*----- Chân 2 (GND) cảm biến Sharp
                         |
                         *----- Chân 4 (GND) cảm biến Sharp
```
*   **Chân 3 (LED)** nối thẳng tới **GPIO 5** của ESP32.
*   **Chân 5 (Vo)** nối thẳng tới **GPIO 4** của ESP32.
*   **Chân 6 (LED GND)** nối tới **GND** chung.

---

## 3. Khuyến nghị Cấp nguồn Hệ thống
Để tránh hiện tượng ESP32 bị khởi động lại (reset) ngẫu nhiên do sụt áp khi MQ135 sấy nhiệt hoặc bóng LED của cảm biến bụi nháy sáng:
1. **Lấy nguồn 5V ngoài:** Sử dụng module nguồn Breadboard MB102 cấp điện 5V độc lập từ Adapter cắm tường.
2. Nối **GND của nguồn ngoài** chung với **GND của ESP32-S3**.
3. Cấp điện 5V của nguồn ngoài vào chân `VCC` của **MQ135** và đầu vào **mạch lọc RC của GP2Y1014**.
4. ESP32-S3 và màn hình ST7789 có thể lấy nguồn trực tiếp từ cổng USB máy tính (khi đang debug) hoặc từ nguồn 3.3V/5V của module MB102.
