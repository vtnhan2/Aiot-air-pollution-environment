# Kế hoạch Tích hợp Màn hình ST7789 (1.54" 240x240)

Để hiển thị giao diện đẹp mắt (UI Apple Watch) lên màn hình ST7789, chúng ta sẽ sử dụng thư viện **TFT_eSPI**. Kế hoạch triển khai như sau:

## User Review Required
> [!IMPORTANT]
> **Sơ đồ đấu dây (Wiring Pinout):**
> Vui lòng xác nhận cấu hình chân cắm (Pinout) dưới đây cho màn hình ST7789. Nếu bạn muốn đổi chân nào, hãy báo lại cho tôi.
> - **MOSI (SDA):** GPIO 11
> - **SCLK (SCL):** GPIO 12
> - **CS:** GPIO 10 (Thường ST7789 1.54" không có chân CS, nếu màn của bạn không có thì bỏ qua)
> - **DC (RS):** GPIO 13
> - **RST (RES):** GPIO 14
> - **BLK (Backlight):** GPIO 15 (Hoặc nối thẳng 3.3V nếu luôn muốn sáng 100%)
> - **VCC:** 3.3V
> - **GND:** GND

## Proposed Changes

### Firmware (ESP32-S3)

#### [MODIFY] [platformio.ini](file:///d:/Work/Outsource_AioT_environment/esp32-air-quality/platformio.ini)
- Thêm thư viện `bodmer/TFT_eSPI`.
- Sử dụng cơ chế cấu hình trực tiếp qua `build_flags` (để tránh việc bạn phải chui vào thư viện sửa file `User_Setup.h` bằng tay). Các flag này sẽ khai báo loại IC là `ST7789`, độ phân giải `240x240` và các chân GPIO.

#### [NEW] `src/display/display_manager.h`
- Định nghĩa class `DisplayManager`.
- Các hàm quản lý UI: `init()`, `drawDashboard()`, `updateData()`.

#### [NEW] `src/display/display_manager.cpp`
- Lập trình giao diện 4 ô (Nhiệt độ, Độ ẩm, Bụi thô, Bụi AI dự đoán).
- Code vòng tròn màu (Xanh/Vàng/Đỏ) bao quanh các chỉ số dựa trên mức độ cảnh báo (Good/Moderate/Unhealthy).

#### [MODIFY] [main.cpp](file:///d:/Work/Outsource_AioT_environment/esp32-air-quality/src/main.cpp)
- `#include "display/display_manager.h"`
- Khởi tạo màn hình trong `setup()`.
- Cập nhật số liệu lên màn hình mỗi khi cảm biến và AI có dữ liệu mới trong `loop()`.

---

## Verification Plan

### Automated Tests
1. Biên dịch lại toàn bộ dự án (`pio run`) để đảm bảo không bị xung đột giữa thư viện TensorFlow Lite và TFT_eSPI (cả hai đều xài PSRAM rất nhiều).

### Manual Verification
1. Bạn nạp code và kiểm tra xem màn hình có sáng lên không.
2. Kiểm tra xem các số liệu (bụi, nhiệt độ) có cập nhật mượt mà trên màn hình không, có bị chớp nháy (flicker) khi vẽ lại hay không.
