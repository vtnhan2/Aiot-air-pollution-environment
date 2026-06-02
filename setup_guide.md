# Hướng dẫn Cài đặt Môi trường (Setup Guide) cho Dự án AIoT Air Quality

Tài liệu này hướng dẫn chi tiết cách cài đặt tất cả các công cụ, phần mềm cần thiết trên máy tính **Windows** để có thể biên dịch, nạp code cho ESP32 và triển khai Website Dashboard lên Firebase.

---

## 1. Cài đặt Visual Studio Code (VSCode) & Git
Đây là trình soạn thảo mã nguồn chính cho toàn bộ dự án (cả code C++ cho ESP32 và code Web).

1. Tải và cài đặt VSCode từ trang chủ: [https://code.visualstudio.com/](https://code.visualstudio.com/)
2. Tải và cài đặt Git (để tải code từ Github): [https://git-scm.com/download/win](https://git-scm.com/download/win)
   - Trong quá trình cài đặt Git, cứ nhấn "Next" để giữ các thiết lập mặc định.

Sau khi cài xong, mở Terminal (PowerShell hoặc CMD) và gõ lệnh sau để tải project về máy (nếu chưa có):
```powershell
git clone https://github.com/vtnhan2/Aiot-air-pollution-environment.git
```

---

## 2. Cài đặt Node.js và Firebase CLI (Cho Web Dashboard)
Node.js là môi trường bắt buộc để chạy lệnh đẩy (deploy) website lên hệ thống máy chủ của Firebase.

1. **Cài đặt Node.js:**
   - Tải bản LTS (Long Term Support) từ: [https://nodejs.org/](https://nodejs.org/)
   - Chạy file cài đặt, nhấn Next liên tục. Nhớ tick chọn ô *"Automatically install the necessary tools"* nếu được hỏi.
2. **Kiểm tra đã cài thành công chưa:** Mở Terminal mới (bắt buộc phải mở lại Terminal để cập nhật biến môi trường) và gõ:
   ```powershell
   node -v
   npm -v
   ```
   *(Nếu hiện ra phiên bản mạng dạng `v20.x.x` là thành công).*

3. **Cài đặt Firebase CLI:**
   - Trong Terminal, gõ lệnh sau để cài đặt công cụ Firebase toàn cục:
   ```powershell
   npm install -g firebase-tools
   ```
4. **Đăng nhập vào tài khoản Google (Firebase):**
   ```powershell
   firebase login
   ```
   - Trình duyệt sẽ mở ra, bạn chọn tài khoản Google đang chứa project Firebase (`aiot-air-quality`). Chọn Allow (Cho phép).

---

## 3. Cài đặt PlatformIO (Cho ESP32-S3)
PlatformIO là công cụ tích hợp ngay trong VSCode giúp biên dịch mã C++ và quản lý thư viện cực kỳ tự động cho mạch ESP32.

1. Mở **VSCode**.
2. Bấm vào biểu tượng **Extensions** (Tiện ích mở rộng) ở cột bên trái (hoặc nhấn `Ctrl + Shift + X`).
3. Gõ vào ô tìm kiếm: `PlatformIO IDE`.
4. Nhấn **Install**. 
   - *Lưu ý: Quá trình cài đặt ngầm có thể mất 3-5 phút tùy mạng, hãy nhìn góc dưới cùng bên phải của VSCode để xem tiến trình. Sau khi cài xong, VSCode có thể yêu cầu **Reload Window** (Khởi động lại).*
5. Bấm nút biểu tượng con kiến (PlatformIO) ở thanh bên trái để kiểm tra xem nó đã hoạt động chưa.

---

## 4. Hướng dẫn Chạy & Nạp Code

### A. Nạp code cho ESP32-S3
1. Dùng VSCode mở thư mục `esp32-air-quality` (File -> Open Folder -> Chọn thư mục `esp32-air-quality`).
2. Cắm cáp USB nối ESP32-S3 với máy tính.
3. Chờ PlatformIO tự động tải các thư viện (BME280, Firebase, TensorFlowLite...) khai báo trong `platformio.ini`.
4. Nhấn biểu tượng **mũi tên hướng sang phải (→)** ở thanh trạng thái dưới cùng của VSCode để biên dịch và Upload code vào vi điều khiển.
5. Nhấn biểu tượng **phích cắm (Serial Monitor)** để xem log debug xem mạch có chạy đúng hay không.

### B. Deploy Web Dashboard lên Internet
1. Mở Terminal trong VSCode, trỏ đường dẫn tới thư mục gốc của project chứa file `firebase.json` (tức là thư mục `Aiot-air-pollution-environment`).
2. Gõ lệnh triển khai (deploy) trang web:
   ```powershell
   firebase deploy --only hosting
   ```
3. Chờ 10 giây, Terminal sẽ trả về đường link web online (ví dụ: `https://aiot-air-quality.web.app`).

---

## 5. Xử lý Lỗi Thường Gặp (Troubleshoot)

- **Lỗi `firebase : The term 'firebase' is not recognized`:** 
  Do bạn chưa cài Node.js, hoặc cài xong nhưng chưa khởi động lại VSCode. Hoặc có thể chạy tạm lệnh `npx firebase-tools deploy --only hosting` để thay thế.
- **Lỗi `Fatal error: Could not open COM18` khi nạp code ESP32:**
  Do cửa sổ Serial Monitor đang mở chiếm cổng COM. Hãy nhấn `Ctrl + C` trong cửa sổ Terminal của Serial Monitor để tắt nó đi rồi mới bấm nút Upload (→) lại.
- **Biểu đồ web đứng yên (Không nhận Realtime):**
  Xóa bộ nhớ đệm trình duyệt bằng cách nhấn `Ctrl + F5`, đảm bảo công tắc góc phải trên web đang gạt sang mục **Realtime Mode**.
