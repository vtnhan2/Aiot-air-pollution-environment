#include "sensor_manager.h"
#include <Wire.h>

SensorManager::SensorManager()
    : dust(PIN_GP2Y_MEASURE, PIN_GP2Y_LED), mq135(PIN_MQ135_MEASURE) {}

void SensorManager::begin() {
  Serial.println("Initializing I2C...");
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  delay(500);

#if USE_REAL_SENSORS
  Serial.println("Initializing BME280...");
  // Pass correct address via wrapper. Wrapper will call Adafruit_BME280::begin
  // which calls Wire.begin() with default pins.
  bme_ok = bme.begin(&Wire);
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL); // Force pins back to 8 and 9
  if (!bme_ok) {
    Serial.println("Failed to init BME280!");
  }
  delay(500);

  Serial.println("Initializing GP2Y1014 Dust Sensor...");
  dust.begin();
  delay(500);

  Serial.println("Initializing MQ135...");
  mq135.begin();
  delay(500);
#else
  Serial.println("[INFO] Running in SENSOR SIMULATION mode.");
#endif

  Serial.println("Sensor initialization finished.");
}

SensorData SensorManager::readAll() {
  SensorData data;
  data.isValid = true;

#if USE_REAL_SENSORS
  // Đọc dữ liệu thực tế từ cảm biến
  if (bme_ok) {
    data.temperature = bme.readTemperature();
    data.humidity = bme.readHumidity();
    data.pressure = bme.readPressure();
  } else {
    // Fallback nếu BME280 bị lỗi khi chạy
    data.temperature = 25.0f;
    data.humidity = 50.0f;
    data.pressure = 1013.25f;
  }

  data.dustDensity = dust.readDustDensity();
  data.gasVoltage = mq135.readGasLevel();
#else
  // Sinh dữ liệu ảo bằng hàm sine để tạo chuỗi dữ liệu mượt mà như thật (chế độ
  // test AI)
  float t = millis() / 1000.0; // Thời gian trôi qua (giây)

  // PM2.5 dao động từ 10 đến 150 ug/m3
  data.dustDensity = 80.0 + 70.0 * sin(t * 0.1);

  // Tạo nhiễu bất thường (Anomaly): 5% cơ hội PM2.5 nhảy vọt lên > 500
  // Để chứng minh thuật toán Anomaly Detection hoạt động!
  if (random(0, 100) < 5) {
    data.dustDensity += 400.0 + random(0, 200);
  }

  // Nhiệt độ dao động từ 22 đến 32 °C
  data.temperature = 27.0 + 5.0 * sin(t * 0.05);

  // Độ ẩm dao động từ 40 đến 80 %
  data.humidity = 60.0 + 20.0 * cos(t * 0.07);

  // Áp suất dao động quanh 1010 hPa
  data.pressure = 1010.0 + 5.0 * sin(t * 0.02);

  // Khí gas
  data.gasVoltage = 1.0 + 0.5 * sin(t * 0.08);
#endif

  return data;
}
