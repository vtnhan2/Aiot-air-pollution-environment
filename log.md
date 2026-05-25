# Nhật ký dự án: Air Quality AIoT (ESP32 Edge Computing)

Tài liệu này lưu lại toàn bộ tiến độ, các kỹ thuật đã áp dụng và kế hoạch phát triển của dự án để tiện cho việc theo dõi và viết báo cáo/thuyết trình.

---

## 1. Các Kỹ thuật và Tính năng Đã Triển khai

### 1.1 Lõi Firmware (C++ / ESP32-S3)
- **Thu thập dữ liệu cảm biến**: Setup BME280 (Nhiệt, Ẩm, Áp suất), SCD40 (CO2), GP2Y1014 (Bụi PM2.5), MQ135 (Khí Gas). Viết module `sensor_manager.cpp` có khả năng sinh dữ liệu ảo (hàm sine) rất giống thực tế để phục vụ test AI khi chưa gắn đủ sensor vật lý.
- **Tiền xử lý (MinMaxScaler)**: Thuật toán đồng bộ biên độ của dữ liệu thô về dải `[0, 1]` chuẩn bị cho AI học.
- **Edge AI với TensorFlow Lite Micro**:
  - Ép mô hình AI (vốn chạy trên server) chạy trực tiếp trên chip ESP32 mà không cần Internet.
  - Tích hợp bộ nhớ ngoài **PSRAM** (`2 MB Tensor Arena`) để cấp đủ RAM cho thuật toán mạng nơ-ron sâu.
  - Dequantize (chuyển đổi ngược) kết quả `INT8` (dữ liệu số nguyên siêu nhẹ) về chỉ số bụi PM2.5 thực tế (`Float32`).
- **Lọc Nhiễu Anomaly Detection**:
  - Không bê nguyên thuật toán Isolation Forest nặng nề từ Python xuống, thay vào đó lập trình thuật toán **Z-Score kết hợp Exponential Moving Average (EMA)**.
  - ESP32 liên tục "học" đường cơ sở của bụi. Bất cứ khi nào bụi nhảy vọt cực đoan (do thổi khói, tạt bụi), nó phát hiện ra ngay lập tức, khóa điểm dữ liệu đó lại để không làm nhiễu AI.
- **Tối ưu phần cứng & Loại bỏ SCD40**:
  - Loại bỏ hoàn toàn cảm biến CO2 SCD40 để tiết kiệm ngân sách (~1.5 triệu VND) do không đóng góp cho việc dự đoán PM2.5 ngoài trời của AI.
  - Chỉ giữ lại: BME280 (Nhiệt/Ẩm/Áp suất), GP2Y1014 (Bụi), và MQ135 (Báo cháy/khí độc).
- **Màn hình ST7789 1.54" IPS (240x240)**:
  - Tích hợp thư viện `TFT_eSPI` cấu hình bằng build flags trực tiếp trong `platformio.ini`.
  - Thiết kế `DisplayManager` với cơ chế vẽ đệm **Double-buffering (TFT_eSprite)** trên **PSRAM** giúp hiển thị giao diện Darkmode cực đẹp dạng "Apple Watch" mà không bị giật, chớp màn hình.

### 1.2 Giao diện (Web Dashboard)
- **Giao diện Modern UI**: Viết bằng HTML/CSS/Vanilla JS thuần, giao diện Glassmorphism (Thẻ kính trong suốt), tone màu tối (Darkmode) chuyên nghiệp.
- **Chart.js Trực quan hóa thuật toán**: Đã lập trình biểu đồ vẽ 3 đường để chứng minh trực quan việc hệ thống đang làm:
  1. Đường đỏ (Raw Sensor): Đầy các gai nhiễu khổng lồ.
  2. Đường xanh dương (Filtered): Mượt mà, vứt bỏ toàn bộ gai nhiễu.
  3. Đường tím (AI Forecast): Đường dự báo tương lai trước 1 giờ.
- **Báo động**: Tích hợp badge cảnh báo `⚠️ Anomaly Detected!` nháy đỏ góc màn hình ngay khoảnh khắc thuật toán phát hiện nhiễu.

### 1.3 Huấn luyện Mô hình AI (Python)
- **`train_lstm.py`**: Pipeline chuẩn bị dữ liệu 시계열 (chuỗi thời gian), train thuật toán **LSTM (Long Short-Term Memory)** dự báo ô nhiễm trước 1-3 giờ.
- **Lượng tử hóa (Quantization)**: Nén kích thước file `.tflite` từ Float32 xuống INT8 (nhẹ hơn 4 lần, tính toán nhanh hơn chục lần cho vi điều khiển), xuất ra file `model_data.cpp` mảng C-array để nhúng vào ROM ESP32.

---

## 2. Lịch sử Cập nhật (Git Log Tóm Tắt)

1. **Khởi tạo dự án AIoT**: Setup PlatformIO, cấu hình bo mạch `esp32s3box`, khai báo các thư viện phụ thuộc (`TensorFlowLite_ESP32`, `Firebase`, `Sensors...`).
2. **Setup AI Inference**: Lắp ráp mã nguồn cấp phát bộ nhớ (Arena), Model Resolver, Interpreter trong `main.cpp`. Cấu hình `ARDUINO_USB_CDC_ON_BOOT` để tương thích cổng Serial COM.
3. **Cấu trúc lại thư mục mạng**: Phân mảnh cấu trúc thành `src/network/` (Chứa WiFi, Firebase, OTA), `src/sensors/` và tạo thư mục độc lập `web-dashboard/`.
4. **`5dddce5`**: `feat(ai): implement Z-score anomaly detection filter in C++ and update web dashboard to visualize anomalies.` (Hoàn thiện AI và bộ lọc nhiễu, cập nhật Web Demo cực xịn).
5. **Cập nhật Mạng, Đám mây & Màn hình**: 
   - Đã viết xong `wifi_manager` và `firebase_manager` cho C++.
   - Đã nâng cấp `web-dashboard` kết nối trực tiếp với Firebase Realtime Database.
   - Gỡ bỏ SCD40 khỏi toàn bộ code, xóa file `scd40.cpp/h`.
   - Viết thành công Driver và UI cho màn hình ST7789 (240x240) sử dụng đệm Sprite, build thử nghiệm thành công 100% không lỗi (`pio run`).
   - Tạo file `hardware_notes.md` hướng dẫn sơ đồ dây chi tiết.

---

## 3. Kế hoạch tiếp theo (Upcoming Plan)

Đây là các hạng mục sẽ làm ở chặng tiếp theo (Trích xuất từ `implementation_plan.md`):

1. **Kết nối Firebase thực tế**: Sau khi chuẩn bị database, bạn điền API Key và Link DB vào `firebase_manager.cpp` và `index.html` của web để đồng bộ số liệu thật.
2. **Lắp ráp và kiểm tra phần cứng**: Tiến hành đấu dây các cảm biến (BME280, GP2Y1014, MQ135), còi buzzer, led và màn hình ST7789 theo đúng file `hardware_notes.md`.
3. **`ota_manager.cpp`**: Kích hoạt ArduinoOTA (tính năng này tạm gác lại theo yêu cầu của bạn, sẽ làm sau).
4. **Vẽ đồ thị nâng cấp**: Tận dụng tối đa cảm biến bụi mịn hoặc các chỉ số khác của BME280 để vẽ thêm đồ thị nhiệt độ/độ ẩm trên Web Dashboard.
