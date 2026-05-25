# Danh sách Linh kiện Đồ án (Bill of Materials)
**Tên đồ án:** Hệ thống giám sát và Dự đoán chất lượng không khí (Air Quality AIoT)

Dựa trên yêu cầu của bạn (file `REQ.md`) và lộ trình hoàn thiện với AI dự đoán (Predictive Control), dưới đây là danh sách linh kiện đầy đủ để bạn chuẩn bị.

## 1. Vi điều khiển trung tâm (Microcontroller)
*   **Mạch phát triển:** Kít phát triển ESP32-S3 (Khuyên dùng loại có 8MB PSRAM, ví dụ: ESP32-S3-WROOM-1 N16R8). 
    *   *Lý do:* ESP32-S3 hỗ trợ AI (Tensorflow Lite Micro) rất tốt nhờ tập lệnh vector và có dư dả RAM (PSRAM) để nạp các mô hình mạng nơ-ron như LSTM/RNN. Có tích hợp sẵn WiFi.

## 2. Các Cảm biến (Sensors)
*   **Cảm biến Bụi mịn PM2.5:** 
    *   **Lựa chọn 1 (Theo REQ.md):** `PMS7003` hoặc `PMS5003` (Giao tiếp UART, độ chính xác rất cao dùng laser).
    *   **Lựa chọn 2 (Đang code sẵn):** `GP2Y1014AU` (Cảm biến quang học Sharp, giá rẻ hơn, dùng chân Analog). *Khuyên dùng PMS7003 cho đồ án tốt nghiệp để số đo chuẩn xác nhất.*
*   **Cảm biến Khí Gas & VOCs:**
    *   `MQ135` (Cảm biến chất lượng không khí, phát hiện NH3, NOx, cồn, Benzen, khói...). Giao tiếp Analog.
*   **Cảm biến Nhiệt độ, Độ ẩm & Áp suất:**
    *   `BME280` (Giao tiếp I2C). Rất chính xác cho môi trường.
*   **Cảm biến CO2 (Tùy chọn thêm để xịn hơn):**
    *   `SCD40` hoặc `SCD41` (Giao tiếp I2C). Đây là cảm biến CO2 chuẩn quang âm học rất xịn, đang được tích hợp sẵn trong code hiện tại của bạn.

## 3. Giao diện Hiển thị (Display)
*   **Màn hình TFT:** `ILI9341` (kích thước 2.4" đến 2.8") hoặc `ST7789` (kích thước 1.54"). 
    *   Giao tiếp: SPI. 
    *   *Tính năng:* Hiển thị màu sắc giao diện Dashboard mượt mà bằng LVGL, vẽ biểu đồ Line chart cho PM2.5 dự đoán.

## 4. Cơ cấu chấp hành & Cảnh báo (Actuators & Alerts)
*   **Module Relay:** Module Relay 1 kênh hoặc 2 kênh (5V). Dùng để điều khiển quạt hút hoặc máy lọc không khí (Tích hợp tính năng AI tự bật quạt trước khi không khí ô nhiễm).
*   **Còi chíp (Buzzer):** Còi chip Active 5V để phát tiếng bíp khi nồng độ ô nhiễm vượt ngưỡng nguy hiểm.
*   **LED Cảnh báo:** 1 bóng LED RGB (như module WS2812B) để nháy màu xanh (An toàn), vàng (Cảnh báo), đỏ (Nguy hiểm).

## 5. Nguồn và Linh kiện phụ trợ
*   **Nguồn cấp:** Adapter 5V - 2A (Cấp nguồn qua cổng Type-C của mạch ESP32) hoặc dùng pin Li-po 3.7V nếu mạch có tích hợp mạch sạc pin.
*   **Dây cắm test board:** Dây cắm đực-cái, đực-đực, đực-cái (Jumper wires).
*   **Test board (Breadboard):** 1-2 cái MB-102 loại lớn để cắm test linh kiện.
*   **Trở & Tụ:** Tùy chọn (vài con điện trở 10k kéo pull-up cho I2C nếu module cảm biến chưa có).

---
> **Ghi chú tiến độ:** Hiện tại trong source code đã có sẵn Driver và class quản lý toàn bộ các cảm biến `PMS/GP2Y`, `MQ135`, `BME280`, `SCD40`. Bộ nhớ AI (PSRAM) và cấu trúc chạy AI đều đã tích hợp xong. Bạn chỉ cần mua đủ linh kiện về cắm đúng chân (Pins) là hệ thống sẽ chạy!
