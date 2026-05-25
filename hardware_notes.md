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
| | SDA | **GPIO 8** | Giao tiếp I2C |
| | SCL | **GPIO 9** | Giao tiếp I2C |
| **Cảm biến Bụi GP2Y1014** | VCC (Chân 1) | 5V (hoặc Nguồn ngoài 5V) | Cần mạch lọc RC (Trở 150 $\Omega$ + Tụ 220uF) |
| | GND (Chân 2 & 4) | GND | Nối chung Ground |
| | LED (Chân 3) | **GPIO 5** | Chân điều khiển xung LED |
| | Vo (Chân 5) | **GPIO 4** | Chân đọc tín hiệu Analog (ADC) |
| **Cảm biến MQ135** | VCC | 5V (Nguồn ngoài khuyên dùng) | Cực kỳ tốn điện (Sấy nóng) |
| | GND | GND | Ground |
| | AO (Analog Out) | **GPIO 1** | Chân đọc tín hiệu Analog (ADC) |
| **Màn hình ST7789 IPS** | VCC | 3.3V | Cấp nguồn 3.3V |
| | GND | GND | Ground |
| | SCL (SCLK) | **GPIO 12** | Giao tiếp SPI Clock |
| | SDA (MOSI) | **GPIO 11** | Giao tiếp SPI Data |
| | RES (Reset) | **GPIO 14** | Chân reset màn hình |
| | DC (RS/Data-Cmd)| **GPIO 13** | Chân chọn Dữ liệu/Lệnh |
| | CS (Chip Select) | **GPIO 10** | (Nối GND nếu màn hình không có chân CS) |
| | BLK (Backlight) | **GPIO 15** | Điều khiển độ sáng màn hình |
| **Còi chip (Buzzer)** | VCC / I/O | **GPIO 6** | Điều khiển bật còi (KY-012/KY-006) |
| | GND | GND | Ground |
| **LED Cảnh báo** | Dương cực (+) | **GPIO 7** | Đèn báo đỏ (Cần thêm trở 220 $\Omega$ hạn dòng) |
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
