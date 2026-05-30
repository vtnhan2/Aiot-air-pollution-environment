# Hướng dẫn thiết lập và các lệnh thao tác với ESP32 (PlatformIO)

## I. Thiết lập môi trường trên máy mới

Để làm việc với project này trên một máy tính mới, bạn có hai cách để cài đặt công cụ PlatformIO:

### Cách 1: Sử dụng VS Code Extension (Khuyên dùng)
1. Tải và cài đặt [Visual Studio Code](https://code.visualstudio.com/).
2. Mở VS Code, vào mục **Extensions** (hoặc nhấn `Ctrl+Shift+X`).
3. Tìm kiếm từ khóa **PlatformIO IDE** và nhấn **Install**.
4. Chờ quá trình cài đặt hoàn tất (sẽ có thông báo yêu cầu reload/khởi động lại VS Code ở góc dưới bên phải).
5. Mở thư mục `esp32-air-quality` trong VS Code. PlatformIO sẽ tự động đọc file `platformio.ini` và tải về các toolchain (trình biên dịch, thư viện) cần thiết cho ESP32.

### Cách 2: Cài đặt PlatformIO Core CLI (Dành cho Terminal/Command Line)
Nếu bạn không muốn dùng VS Code mà chỉ cần bộ lệnh gõ terminal độc lập:
1. Đảm bảo máy đã cài đặt [Python 3](https://www.python.org/downloads/) (nhớ tick chọn *"Add Python to PATH"* khi cài đặt).
2. Mở Terminal / Command Prompt / PowerShell và chạy lệnh cài đặt:
   ```bash
   pip install -U platformio
   ```
3. Xác minh đã cài đặt thành công:
   ```bash
   pio --version
   ```

---

## II. Các lệnh cơ bản thao tác với ESP32

Dự án này sử dụng **PlatformIO** để biên dịch và nạp code cho ESP32. Để thực thi các lệnh dưới đây, bạn cần mở terminal và di chuyển vào thư mục chứa code ESP32 (`esp32-air-quality`):

```bash
cd esp32-air-quality
```

Dưới đây là các lệnh cơ bản (PlatformIO Core CLI):

## 1. Biên dịch dự án (Build)
Lệnh này sẽ biên dịch mã nguồn mà không nạp vào board. Sử dụng để kiểm tra lỗi cú pháp trước khi nạp.
```bash
pio run
```

## 2. Nạp code vào ESP32 (Flash/Upload)
Lệnh này sẽ biên dịch (nếu có thay đổi) và nạp firmware vào board ESP32. PlatformIO sẽ tự động dò tìm cổng COM đang kết nối.
```bash
pio run --target upload
```
Hoặc viết tắt:
```bash
pio run -t upload
```

Nếu bạn có nhiều thiết bị đang cắm hoặc muốn chỉ định rõ cổng COM (ví dụ cổng `COM3`):
```bash
pio run -t upload --upload-port COM3
```

## 3. Hiển thị dữ liệu từ ESP32 (Serial Monitor)
Lệnh này dùng để mở giao diện theo dõi cổng Serial (Serial Monitor) để xem các log/thông báo được in ra từ ESP32 (như `Serial.print`).
```bash
pio device monitor
```
*(Baudrate sẽ được lấy từ cấu hình `monitor_speed` trong tệp `platformio.ini`)*.
Để thoát chế độ Serial Monitor, nhấn `Ctrl + C`.

Nếu muốn chỉ định rõ cổng COM để monitor:
```bash
pio device monitor --port COM3
```

## 4. Vừa nạp code vừa mở Serial Monitor (Lệnh kết hợp)
Thường thì sau khi nạp code xong, chúng ta muốn xem ngay output.
```bash
pio run -t upload && pio device monitor
```

## 5. Dọn dẹp thư mục biên dịch (Clean)
Xóa các file biên dịch cũ để bắt đầu biên dịch lại từ đầu. Rất hữu ích khi bạn đổi phiên bản thư viện hoặc gặp lỗi biên dịch không rõ nguyên nhân.
```bash
pio run --target clean
```

## 6. Xóa toàn bộ Flash của ESP32 (Erase Flash)
Nếu ESP32 bị lỗi crash/reboot vòng lặp liên tục, hoặc bạn muốn xóa sạch thông tin cấu hình (Wi-Fi, thông tin bộ nhớ SPIFFS/LittleFS/EEPROM cũ):
```bash
pio run --target erase
```

## 7. Cập nhật thư viện
Tải về và cập nhật các thư viện được khai báo ở phần `lib_deps` trong file `platformio.ini`:
```bash
pio pkg update
```

---

**💡 Lưu ý quan trọng**: 
- Các lệnh `pio` hoạt động nếu bạn đã thêm PlatformIO Core vào biến môi trường hệ thống (`PATH`). 
- Nếu gõ báo lỗi *'pio' is not recognized...*, bạn hãy dùng **PlatformIO Core CLI** được tích hợp sẵn trong VS Code (Nhấn vào biểu tượng con kiến PlatformIO ở thanh công cụ bên trái > Kéo xuống phần `Miscellaneous` > Chọn `New Terminal`, hoặc biểu tượng Terminal ở thanh trạng thái dưới cùng).
