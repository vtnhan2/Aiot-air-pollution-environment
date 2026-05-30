# Giải thích Chuyên sâu Hệ thống AIoT Giám sát & Dự báo Chất lượng Không khí (Edge AI)

---

## Phần 1: Giải mã chi tiết cấu trúc Báo cáo Đồ án (`report.tex`)

Báo cáo được cấu trúc đi từ vĩ mô (vấn đề xã hội) đến vi mô (từng thuật toán trên vi điều khiển). Sự tồn tại của từng chương phản ánh tư duy hệ thống (Systems Engineering):

### 1. Chương 1: Đặt vấn đề và Tầm nhìn Hệ thống
- **Mục tiêu cốt lõi:** Chuyển đổi mô hình trạm quan trắc từ **thụ động (Passive Monitoring)** sang **chủ động dự báo (Proactive Forecasting)**. 
- **Lý do lựa chọn kiến trúc:** Các hệ thống IoT truyền thống (Cloud-based IoT) bộc lộ nhược điểm chí mạng: độ trễ cao, phụ thuộc băng thông, rủi ro mất kết nối và tốn kém chi phí máy chủ đám mây. Đồ án giải quyết bằng cách áp dụng mô hình **Edge AI (Trí tuệ Nhân tạo tại biên)**, nơi vi điều khiển ESP32-S3 đóng vai trò như một bộ não siêu nhỏ, xử lý dữ liệu và ra quyết định cảnh báo độc lập mà không cần gửi dữ liệu thô (raw data) lên mạng internet.

### 2. Chương 2: Thiết kế Hệ thống và Lựa chọn Thiết bị (Hardware BOM)
Kiến trúc phần cứng được chia làm 3 tầng sinh thái:
- **Tầng Biên (Edge Layer):** Sử dụng chip **ESP32-S3-WROOM-1 (N16R8)**. Việc lựa chọn dòng chip này là quyết định kỹ thuật then chốt. Với 16MB Flash và 8MB PSRAM (RAM ngoài giao tiếp qua bus OPI), nó cung cấp đủ bộ nhớ Heap để cấp phát `Tensor Arena` 2MB cho TensorFlow Lite Micro, điều mà các dòng Arduino thông thường (chỉ có vài chục KB RAM) không thể làm được.
- **Tầng Cảm biến (Sensor Layer):** Tích hợp đa cảm biến để đo lường chéo: 
  - *Sharp GP2Y1014* (Quang học hồng ngoại) đo bụi PM2.5.
  - *BME280* (I2C) đo Nhiệt độ, Độ ẩm, Áp suất để bù trừ sai số môi trường.
  - *MQ135* (Bán dẫn) đo khí VOCs/Gas.
- **Tầng Hiển thị & Cảnh báo:** Sử dụng màn hình ST7789 IPS 240x240px qua giao tiếp SPI với cơ chế **DMA (Direct Memory Access)** và **Sprite Double-Buffering** để tránh hiện tượng giật lag màn hình (flickering). Hệ thống cơ cấu chấp hành (Buzzer + LED) phát cảnh báo vật lý.

### 3. Chương 3 & 4: Khung Thuật toán AI và Cloud
- Mô tả dòng chảy của dữ liệu (Data Pipeline): Tiền xử lý (MinMaxScaler) $\rightarrow$ Lọc nhiễu tĩnh học (Z-Score & EMA) $\rightarrow$ Trượt bộ đệm (Sliding Window) $\rightarrow$ Suy luận (AI Inference) $\rightarrow$ Hậu xử lý (Dequantize).
- Cơ chế Cloud 2 tầng: Dữ liệu gửi lên Firebase Realtime Database được tách bạch thành `/sensor` (realtime 5s) để hiển thị Dashboard và `/history` (60s) để tối ưu hóa dung lượng lưu trữ dài hạn.

---

## Phần 2: Phân tích luồng hoạt động của các Sơ đồ Hệ thống

### 2.1. Sơ đồ Mạch lọc RC bảo vệ cảm biến Sharp GP2Y1014
Bản chất của cảm biến quang học Sharp GP2Y là nháy một đèn LED hồng ngoại liên tục (được điều khiển bởi xung PWM 10ms từ ESP32). Mỗi lần LED bật, nó rút một dòng điện đỉnh (peak current) cực kỳ lớn.

- **Vấn đề (The Problem):** Nếu nối trực tiếp nguồn 5V, dòng đỉnh này tạo ra sự sụt giảm điện áp cục bộ (**Voltage Dip / Ripple**). Sự bất ổn định này dội ngược về chân nguồn của ESP32, làm điện áp tham chiếu của bộ chuyển đổi Tương tự-Số (ADC) bị sai lệch, dẫn đến đọc sai giá trị PM2.5.
- **Giải pháp - Mạch lọc RC (Low-pass Filter):**
  - **Tụ điện hóa ($C = 220\,\mu\text{F}$):** Hoạt động như một "hồ chứa năng lượng" siêu nhỏ. Khi LED chớp, dòng điện được rút ra từ năng lượng dự trữ trong tụ thay vì kéo dòng từ mạch nguồn chính của ESP32.
  - **Điện trở ($R = 150\,\Omega$):** Hạn dòng sạc lại cho tụ, tạo ra hằng số thời gian $\tau = R \cdot C = 33\,\text{ms}$, giúp bộ lọc cản các xung tần số cao dội ngược về nguồn.
  > **Kết luận:** Đây là bước thiết kế phần cứng bắt buộc để đảm bảo tín hiệu đầu vào cho AI là sạch và ổn định về mặt điện học.

### 2.2. Sơ đồ Luồng hoạt động Phần mềm (Flowchart/Lifecycle)
Luồng xử lý (Lifecycle) của firmware C++ chia làm hai pha chính:

1. **Pha Khởi tạo (Setup Phase):**
   - Khởi tạo phần cứng (Serial, I2C, SPI).
   - **Bẫy nút nhấn BOOT (3s):** Xử lý ngoại lệ. Nếu mạng WiFi cũ bị mất, người dùng giữ nút BOOT, hệ thống gọi `WiFiManager` để tự động phát Access Point (`AirQuality_AIoT_Setup`), cho phép cấu hình WiFi mới qua giao diện Web Captive Portal.
   - **Khởi tạo AI:** Dùng lệnh `heap_caps_malloc` cấp phát vùng nhớ PSRAM (RAM ngoài), tải ma trận trọng số INT8 vào `Interpreter` của TensorFlow Lite.

2. **Pha Vòng lặp Suy luận (Inference Loop - 5s/chu kỳ):**
   - **Thu thập (Acquisition):** Lấy dữ liệu 4 chiều (Nhiệt, Ẩm, Áp suất, Bụi thô).
   - **Xử lý nhiễu (Anomaly Blocking):** Đi qua khối Z-Score. Nếu dị thường (gai lớn) $\rightarrow$ Thay bằng giá trị EMA cũ.
   - **Chuẩn hóa & Trượt (Scale & Shift):** Chuyển dải giá trị vật lý (VD: $990 \rightarrow 1050$ hPa) về miền $[0, 1]$ qua `MinMaxScaler`. Sau đó, đẩy cửa sổ thời gian (24 bước) sang trái, nạp mẫu mới vào cuối.
   - **Thực thi AI (Inference):** Hàm `interpreter->Invoke()` đẩy ma trận qua mạng LSTM. Nhờ lượng tử hóa (Quantization), phép toán nhân ma trận này hoàn tất cực kỳ nhanh chóng (chỉ mất $\mathbf{216\,ms}$).
   - **Ngắt & Cảnh báo (Actuation):** Giải chuẩn hóa kết quả. Đưa vào khối Decision. Nếu PM2.5 dự báo $\ge 80\,\mu\text{g/m}^3$, vi điều khiển lập tức băm xung PWM ra còi hú và bật LED cảnh báo.
   - **Đồng bộ (Sync):** Đẩy gói tin JSON mã hóa SSL/TLS lên Firebase.

---

## Phần 3: Cơ sở Lý thuyết Học thuật & Nguyên lý Edge AI

### 3.1. Nghịch lý Cloud Computing và Cứu cánh Edge AI
Các mô hình học sâu truyền thống (Deep Learning) đòi hỏi năng lực tính toán GPU khổng lồ. Tuy nhiên, gửi liên tục mẫu cảm biến (mỗi 5 giây) lên Cloud gây ra **Nghịch lý Băng thông và Độ trễ**.
- **Edge AI** đảo ngược mô hình này: Thay vì đưa dữ liệu đến nơi có AI, ta nén mô hình AI và đưa nó đến nơi sinh ra dữ liệu. Lợi ích học thuật đạt được bao gồm:
  1. **Deterministic Latency (Độ trễ tất định):** Thời gian suy luận luôn cố định ở $216\,\text{ms}$, không phụ thuộc vào tình trạng cáp quang biển.
  2. **Fault Tolerance (Khả năng chịu lỗi):** Hệ thống có tính sống còn cao. Ngay cả khi Router mạng bị cắt điện, khối suy luận AI và còi báo động vẫn hoạt động độc lập (offline inference).

### 3.2. Toán học của LSTM và Lượng tử hóa INT8 (Quantization)
- **Kiến trúc LSTM:** Chất lượng không khí là bài toán chuỗi thời gian (time-series). LSTM giải quyết hiện tượng *Vanishing Gradient (Tiêu biến đạo hàm)* của mạng RNN truyền thống nhờ cấu trúc "Cell State". LSTM sử dụng các Cổng quên (Forget Gate: $f_t$), Cổng vào (Input Gate: $i_t$) để quyết định chính xác xem nên nhớ sự kiện "đám khói bốc lên lúc 10 phút trước" và quên đi "hạt bụi thoáng qua" như thế nào.
- **Lượng tử hóa (Post-Training Quantization):** Trọng số mạng LSTM vốn là số thực 32-bit (Float32). ESP32 không có FPU (Bộ xử lý dấu phẩy động) mạnh mẽ. Việc ép dải Float32 xuống dải số nguyên 8-bit (INT8, từ -128 đến 127) mang lại 2 lợi ích:
  - **Giảm $4$ lần dung lượng RAM/Flash** (Model cực nhẹ, dễ dàng nhúng vào C array `model_data.h`).
  - Phép nhân ma trận CPU chuyển từ dấu phẩy động sang phép dịch bit/số nguyên (Integer Arithmetic), giúp **tốc độ tăng gấp nhiều lần**.

### 3.3. Thuật toán Tiền xử lý Dị thường (Z-Score + EMA)
Cảm biến bụi rẻ tiền bị một điểm yếu chí mạng: một con côn trùng bay qua hay luồng gió thổi bụi đột ngột tạo ra "gai nhiễu" tức thời lên tới $500\,\mu\text{g/m}^3$. Nếu đưa thẳng vào LSTM, bộ nhớ mô hình sẽ bị "ngộ độc" (Data Poisoning) trong nhiều chu kỳ.

Đồ án ứng dụng mô hình thống kê kết hợp:
1. **EMA (Đường trung bình động lũy thừa):** Duy trì một đường cơ sở (baseline) mượt mà bằng công thức $EMA_t = 0.2 \cdot x_t + 0.8 \cdot EMA_{t-1}$.
2. **Z-Score động:** Tính khoảng cách của điểm đo hiện tại so với $EMA$ theo đơn vị độ lệch chuẩn $\sigma$. 
   $$Z_t = \frac{|x_t - EMA_{t-1}|}{\sigma_t}$$
   Dựa vào tính chất phân phối chuẩn Gauss, nếu $Z_t > 3.0$, mẫu đo nằm ở miền $0.3\%$ ngoại lai (Outlier). Lúc này, thuật toán **khóa dữ liệu (block)**, loại bỏ gai nhiễu và thay thế bằng giá trị $EMA$ cũ để duy trì mạch thời gian liên tục cho AI.

---

## Phần 4: Ý nghĩa Kỹ thuật của các Thông số AI Dự báo

Mô hình học máy trong dự án xuất ra một giá trị duy nhất (Scalar Output): **Dự báo nồng độ bụi PM2.5 trong 1 giờ kế tiếp**. Thông số này đóng vai trò quyết định cấu trúc an toàn của hệ thống (System Safety Architecture).

### 4.1. Cơ chế Ngưỡng (Thresholding) & Hệ quả Hành vi
| Ngưỡng dự báo              | Phân loại       | Hành động Can thiệp Hệ thống (Actuation)                                                   | Ý nghĩa Kỹ thuật                                                                                                    |
| :------------------------- | :-------------- | :----------------------------------------------------------------------------------------- | :------------------------------------------------------------------------------------------------------------------ |
| $\le 35\,\mu\text{g/m}^3$  | Bình thường     | Chỉ hiển thị trên LCD và Web.                                                              | Vùng hoạt động tối ưu.                                                                                              |
| $35 - 79\,\mu\text{g/m}^3$ | Cảnh báo sớm    | Giao diện Web chuyển trạng thái (UI thay đổi).                                             | Cảnh báo xu hướng tăng (Trend Alert). Giúp người dùng đóng cửa kính hoặc bật máy lọc khí trước khi ô nhiễm tích tụ. |
| $\ge 80\,\mu\text{g/m}^3$  | **Mức Độc hại** | Vi điều khiển nổ ngắt phần cứng, băm xung còi Buzzer ở tần số chói (KY-012) và bật LED đỏ. | Phản ứng khẩn cấp (Emergency Actuation).                                                                            |

### 4.2. Tác động của Sai số tới Độ tin cậy (Reliability) và An toàn (Safety)
1. **Safety - Lợi ích của sự Chủ động (Proactive Forecasting):** 
   Nếu không có AI dự đoán, một hệ thống thuần túy "chờ đến lúc đo được $\ge 80$ mới hú còi" thì con người đã trực tiếp hít phải khói độc. Dự đoán trước 1 giờ giúp thiết lập một **"Vùng đệm Thời gian" (Time-buffer)** cho phép con người thực hiện các thao tác di tản hoặc ngăn chặn. Sự an toàn (Safety) được nâng lên từ cấp độ *Phản ứng (Reactive)* sang cấp độ *Ngăn ngừa (Preventive)*.

2. **Reliability - Hiểm họa từ Báo động giả (False Positives):** 
   Đây là thách thức kỹ thuật lớn nhất. Nếu mô hình lượng tử hóa INT8 bị suy giảm độ chính xác lớn (chỉ số MAE/RMSE cao), hoặc khâu lọc nhiễu Z-Score bị sai sót, AI sẽ phán đoán sai và hú còi khi phòng không có bụi. 
   > **Hội chứng Nhờn cảnh báo (Alarm Fatigue):** Một hệ thống báo động giả quá nhiều (False Positive Rate cao) sẽ khiến người dùng bực tức, rút phích cắm hoặc phớt lờ cảnh báo trong tương lai. Tính tin cậy (Reliability) bị phá hủy hoàn toàn.
   
   Đó là lý do TẠI SAO đồ án bắt buộc phải kết hợp bộ lọc Z-Score nghiêm ngặt ở lớp tiền xử lý để "làm sạch" dữ liệu vào, qua đó ép tỷ lệ False Positive xuống tiệm cận 0, biến sản phẩm từ một món đồ chơi điện tử thành một thiết bị có giá trị thực tiễn.
