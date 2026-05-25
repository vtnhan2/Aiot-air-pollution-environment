# Kế hoạch Kiến trúc: Cloud, OTA & Web Dashboard

Dựa trên lựa chọn của bạn, chúng ta sẽ mở rộng dự án để hệ thống có khả năng kết nối Internet vạn vật (IoT), tự động cập nhật từ xa (OTA) và có một giao diện Web trực quan. Dưới đây là quy hoạch cấu trúc thư mục (Folder Structure) và kiến trúc hệ thống.

## 1. Quy hoạch Thư mục (Folder Structure)

Chúng ta sẽ cấu trúc lại thư mục dự án để tách biệt rõ ràng giữa Firmware (C++ chạy trên ESP32) và Frontend (Code Web chạy trên trình duyệt).

```text
Outsource_AioT_environment/
│
├── esp32-air-quality/          <-- Nơi chứa Firmware cho ESP32
│   ├── src/
│   │   ├── ai/                 (Đã có) Xử lý TFLite
│   │   ├── sensors/            (Đã có) Quản lý Cảm biến
│   │   ├── network/            [MỚI] Module mạng và đám mây
│   │   │   ├── wifi_manager.h/cpp       # Quản lý kết nối và tự động kết nối lại WiFi
│   │   │   ├── firebase_manager.h/cpp   # Giao tiếp với Firebase Realtime Database
│   │   │   └── ota_manager.h/cpp        # Quản lý luồng cập nhật Firmware qua WiFi (ArduinoOTA)
│   │   └── main.cpp
│   └── platformio.ini          # (Sẽ thêm thư viện FirebaseClient và ArduinoOTA)
│
└── web-dashboard/              <-- [MỚI] Nơi chứa Giao diện Web
    ├── index.html              # Khung giao diện Web
    ├── style.css               # Code thiết kế (Dark mode, Glassmorphism)
    └── app.js                  # Code JS kết nối Firebase để kéo dữ liệu realtime và vẽ biểu đồ
```

## 2. Thiết kế Module (Firmware)

### A. Network Manager (`wifi_manager`)
- Tách phần code kết nối WiFi từ `main.cpp` ra đây.
- Thêm cơ chế **Auto Reconnect**: Mất mạng tự kết nối lại để đảm bảo ESP32 luôn online liên tục nhiều ngày.

### B. Firebase Manager (`firebase_manager`)
- Sử dụng thư viện `Firebase Arduino Client` của mobizt.
- Cấu trúc JSON dữ liệu gửi lên Firebase Realtime Database:
  ```json
  {
    "current_readings": {
      "temperature": 28.5,
      "humidity": 75.0,
      "pm25": 116.0
    },
    "ai_prediction": {
      "pm25_next_hour": 99.8,
      "timestamp": 1690000000
    }
  }
  ```
- Chu kỳ gửi: Mỗi 1-5 phút một lần để tiết kiệm băng thông mạng.

### C. OTA Manager (`ota_manager`)
- Sử dụng thư viện chuẩn `ArduinoOTA`.
- Chức năng: Mở cổng mạng (Port 3232). Khi bạn bấm nút `Upload` trên PlatformIO, code sẽ bay qua sóng WiFi nạp thẳng vào ESP32 mà không cần cắm cáp USB.

## 3. Thiết kế Web Dashboard
- **Công nghệ:** HTML5, Vanilla CSS, Vanilla JS. (Nhẹ, không cần cài đặt nodejs).
- **Giao diện:**
  - 1 Bảng điều khiển tổng quan (Tổng hợp Nhiệt độ, Độ ẩm, Bụi).
  - 1 Thẻ (Card) nổi bật hiển thị "AI Predicts PM2.5 in 1 Hour".
  - 1 Biểu đồ lịch sử dạng đường (Dùng thư viện `Chart.js`) để theo dõi diễn biến không khí.

---

> [!IMPORTANT]
> ## User Review Required
> Bạn có đồng ý với Cấu trúc thư mục và Kiến trúc này không?
> Nếu bạn "Ok", tôi sẽ dùng lệnh để tự động tạo ra tất cả các thư mục và file trống này, sau đó chúng ta sẽ code từng file một.
