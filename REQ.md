" Hệ thống giám sát và Dự đoán chất lượng không khí (Air Quality AIoT) 
-        Thay vì chỉ hiển thị các chỉ số như PM2.5, CO2, hệ thống này sẽ dùng AI để đưa ra cảnh báo sớm hoặc phân tích nguồn gây ô nhiễm. 
-        Phần cứng: ESP32/rasbery pi 4b/ Raspberry Pi Zero 2 W, cảm biến bụi mịn PM2.5 (PMS7003), cảm biến khí gas (MQ series), cảm biến nhiệt độ/độ ẩm (BME280). 
-        Xử lý AI (Edge Computing): Sử dụng mô hình LSTM (Long Short-Term Memory) hoặc RNN để dự báo nồng độ ô nhiễm trong 1-3 giờ tới dựa trên dữ liệu lịch sử. 
-        Phát hiện bất thường (Anomaly Detection) để phân biệt giữa ô nhiễm môi trường thực sự và nhiễu cảm biến (ví dụ: bụi do quét nhà ngay cạnh cảm biến). 
-        Ứng dụng: Lắp đặt tại các nút giao thông hoặc khu công nghiệp, gửi thông báo qua ứng dụng điện thoại khi mức độ ô nhiễm sắp vượt ngưỡng.
"



