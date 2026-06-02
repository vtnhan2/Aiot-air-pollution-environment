#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include <Arduino.h>
#include <TensorFlowLite_ESP32.h>
#include <WiFi.h>

// Includes cho Sensor và AI
#include "ai/model_data.h"
#include "sensors/sensor_manager.h"

// Includes cho Mạng và Đám mây
#include "display/display_manager.h"
#include "network/firebase_manager.h"
#include "network/wifi_manager.h"

// Cấu hình Database (Đặt thành false nếu chưa setup Firebase để tránh bị spam
// lỗi API Key trên Serial Monitor)
#define USE_FIREBASE true

// Định nghĩa chân Còi Buzzer và LED cảnh báo (Dùng chung 1 chân)
#define PIN_BUZZER 18
#define PIN_LED_WARN 18

SensorManager sensorManager;
DisplayManager displayManager;

// TFLite Globals
const tflite::Model *model = nullptr;
tflite::MicroInterpreter *interpreter = nullptr;
TfLiteTensor *input = nullptr;
TfLiteTensor *output = nullptr;

// Feature scaling constants (MinMaxScaler matching Python training config)
const float FEATURE_MIN[4] = {0.0f, -40.0f, -20.0f, 990.0f};
const float FEATURE_MAX[4] = {1000.0f, 30.0f, 45.0f, 1050.0f};

// Helper function to scale raw data to [0, 1]
float scaleData(float value, int feature_index) {
  if (isnan(value))
    value = FEATURE_MIN[feature_index]; // Default to MIN if NaN
  if (value < FEATURE_MIN[feature_index])
    value = FEATURE_MIN[feature_index];
  if (value > FEATURE_MAX[feature_index])
    value = FEATURE_MAX[feature_index];
  return (value - FEATURE_MIN[feature_index]) /
         (FEATURE_MAX[feature_index] - FEATURE_MIN[feature_index]);
}

// Helper function to inverse scale [0, 1] back to raw value
float inverseScale(float scaled_value, int feature_index) {
  return scaled_value *
             (FEATURE_MAX[feature_index] - FEATURE_MIN[feature_index]) +
         FEATURE_MIN[feature_index];
}

// --- ANOMALY DETECTION (Z-Score Based) ---
float ema_pm25 = -1.0f; // Exponential Moving Average
float ema_variance = 0.0f;
const float EMA_ALPHA = 0.2f; // Learning rate (0-1)
const float ANOMALY_THRESHOLD_Z =
    3.0f; // 3 standard deviations for 99.7% confidence

bool isAnomaly(float current_pm25) {
  if (ema_pm25 < 0) { // First reading initialization
    ema_pm25 = current_pm25;
    return false;
  }

  float diff = current_pm25 - ema_pm25;
  float stddev = sqrt(ema_variance);
  if (stddev < 10.0f)
    stddev = 10.0f; // Base variance floor to prevent division by zero for
                    // stable signals

  float z_score = abs(diff) / stddev;

  if (z_score > ANOMALY_THRESHOLD_Z) {
    return true; // Sudden spike detected!
  }

  // Normal reading -> Update EMA and Variance
  ema_pm25 = (EMA_ALPHA * current_pm25) + ((1.0f - EMA_ALPHA) * ema_pm25);
  ema_variance =
      (EMA_ALPHA * diff * diff) + ((1.0f - EMA_ALPHA) * ema_variance);
  return false;
}
// -----------------------------------------

// Allocate memory for the tensor arena
constexpr int kTensorArenaSize =
    2 * 1024 * 1024; // 2MB in PSRAM for LSTM and weights
// Put arena in PSRAM if available, otherwise use normal RAM
uint8_t *tensor_arena = nullptr;

// Circular buffer for 24 hours (we use simulated quick steps here for testing)
int8_t input_buffer[24][4] = {0};
int step_count = 0;
void setup() {
  Serial.begin(115200);
  delay(1000);

// Tắt đèn RGB LED trên mạch ESP32-S3 (Chân 48) để đỡ chói mắt
#ifdef RGB_BUILTIN
  neopixelWrite(RGB_BUILTIN, 0, 0, 0);
#else
  neopixelWrite(48, 0, 0, 0);
#endif

  Serial.println("\n--- AIoT Air Quality System Starting ---");

  // Khởi tạo các cảm biến
  sensorManager.begin();

  // Cấu hình chân còi và LED cảnh báo
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED_WARN, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED_WARN, LOW);

#if USE_FIREBASE
  // Khởi tạo WiFi (Sử dụng cổng cấu hình tự động khi mất kết nối)
  wifiManager.init();

  // Khởi tạo Firebase
  Serial.println("[DEBUG] Calling firebaseManager.init()...");
  firebaseManager.init();
  Serial.println("[DEBUG] firebaseManager.init() finished.");
#else
  Serial.println(
      "[INFO] Firebase is disabled in code. Running in offline mode.");
#endif

  // Khởi tạo màn hình
  Serial.println("[DEBUG] Calling displayManager.begin()...");
  displayManager.begin();
  Serial.println("[DEBUG] displayManager.begin() finished.");

  // Check if PSRAM is initialized
  if (psramFound()) {
    Serial.printf("PSRAM initialized successfully. Size: %d bytes\n",
                  ESP.getPsramSize());
  } else {
    Serial.println("Warning: PSRAM not found!");
  }

  Serial.println("\n--- Initializing AI Model (TFLite) ---");
  tflite::InitializeTarget();

  // Allocate arena in PSRAM
  tensor_arena =
      (uint8_t *)heap_caps_malloc(kTensorArenaSize, MALLOC_CAP_SPIRAM);
  if (!tensor_arena) {
    Serial.println(
        "Failed to allocate tensor arena in PSRAM! Trying Internal RAM...");
    tensor_arena = (uint8_t *)malloc(kTensorArenaSize);
  }

  model = tflite::GetModel(model_data);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.printf("Model schema mismatch! Expected %d but got %d\n",
                  TFLITE_SCHEMA_VERSION, model->version());
    return;
  }

  static tflite::AllOpsResolver resolver;
  static tflite::MicroErrorReporter micro_error_reporter;
  tflite::ErrorReporter *error_reporter = &micro_error_reporter;

  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
  interpreter = &static_interpreter;

  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    Serial.println("AllocateTensors() failed");
    return;
  }

  input = interpreter->input(0);
  output = interpreter->output(0);

  Serial.printf("AI Model initialized! Input shape: [");
  for (int i = 0; i < input->dims->size; i++) {
    Serial.printf("%d%s", input->dims->data[i],
                  (i == input->dims->size - 1) ? "" : ", ");
  }
  Serial.println("]");

  Serial.println("System setup complete.");
  
  // Hiển thị màn hình khởi động (chứa link web) khoảng 3 giây trước khi vào loop
  displayManager.showBootScreen();
  delay(3000);
}

void loop() {
  SensorData data = sensorManager.readAll();

  Serial.println("\n--- Sensor Readings ---");
  Serial.printf("Temperature: %.2f °C\n", data.temperature);
  Serial.printf("Humidity: %.2f %%\n", data.humidity);
  Serial.printf("Pressure: %.2f hPa\n", data.pressure);
  Serial.printf("Dust (PM2.5 approx): %.2f ug/m3\n", data.dustDensity);
  Serial.printf("Gas Voltage (MQ135): %.2f V\n", data.gasVoltage);

// Check WiFi status in loop (Chỉ check nếu bật Firebase)
#if USE_FIREBASE
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi Status: Connected");
  } else {
    Serial.println("WiFi Status: Disconnected");
  }
#endif

  // --- AI Inference ---
  if (input != nullptr) {
    // Shift buffer to the left (simulating sliding window of 24 timesteps)
    for (int i = 0; i < 23; i++) {
      for (int j = 0; j < 4; j++) {
        input_buffer[i][j] = input_buffer[i + 1][j];
      }
    }

    SensorData data = sensorManager.readAll();

    // --- 1. ANOMALY DETECTION FILTER ---
    bool anomaly_detected = false;
    float raw_pm25 = data.dustDensity; // Save raw for logging

    if (isAnomaly(data.dustDensity)) {
      Serial.printf("\n[⚠️ ANOMALY DETECTED] Sudden PM2.5 spike: %.1f ug/m3. "
                    "Filtered out!\n",
                    data.dustDensity);
      data.dustDensity =
          ema_pm25; // Override noisy data with stable moving average
      anomaly_detected = true;
    }
    // -----------------------------------

    // Add new reading to the end of the buffer
    // 2. Scale raw values to [0, 1] using MinMaxScaler
    float scaled_pm25 = scaleData(data.dustDensity, 0);
    float scaled_dewp =
        scaleData(data.humidity, 1); // Using humidity as DEWP proxy for now
    float scaled_temp = scaleData(data.temperature, 2);
    float scaled_pres = scaleData(data.pressure, 3);

    // 2. Quantize values to int8_t using TFLite tensor parameters
    input_buffer[23][0] =
        (int8_t)(scaled_pm25 / input->params.scale + input->params.zero_point);
    input_buffer[23][1] =
        (int8_t)(scaled_dewp / input->params.scale + input->params.zero_point);
    input_buffer[23][2] =
        (int8_t)(scaled_temp / input->params.scale + input->params.zero_point);
    input_buffer[23][3] =
        (int8_t)(scaled_pres / input->params.scale + input->params.zero_point);

    // Copy buffer to TFLite input tensor
    for (int i = 0; i < 24; i++) {
      for (int j = 0; j < 4; j++) {
        input->data.int8[i * 4 + j] = input_buffer[i][j];
      }
    }

    // Run inference
    unsigned long start_time = millis();
    TfLiteStatus invoke_status = interpreter->Invoke();
    unsigned long inference_time = millis() - start_time;

    if (invoke_status == kTfLiteOk) {
      int8_t predicted_val = output->data.int8[0];

      // 3. Dequantize output back to [0, 1] float
      float dequantized_val =
          (predicted_val - output->params.zero_point) * output->params.scale;

      // 4. Inverse scale to get actual PM2.5 concentration (ug/m3)
      float final_pm25 = inverseScale(dequantized_val, 0);

      Serial.printf("--> AI Prediction (PM2.5 next hour): %.2f ug/m3\n",
                    final_pm25);
      Serial.printf("--> Inference time: %lu ms\n", inference_time);

      if (anomaly_detected) {
        Serial.println("--> [INFO] Prediction was made using filtered data to "
                       "ensure reliability.");
      }

      // Cập nhật lên màn hình LCD ST7789
      displayManager.updateData(
          data.temperature, data.humidity, raw_pm25,
          data.dustDensity, // Đã được lọc nhiễu nếu có anomaly
          final_pm25,       // Dự báo của AI
          data.gasVoltage, (WiFi.status() == WL_CONNECTED), anomaly_detected);

      // --- 6. LOGIC CẢNH BÁO CÒI & LED (Dự báo AI quá cao) ---
      static bool buzzer_active = false;
      if (final_pm25 >= 80.0f) {
        // Đóng còi cảnh báo tần số 1000Hz trong 1 giây (không chặn luồng)
        tone(PIN_BUZZER, 1000, 1000);
        digitalWrite(PIN_LED_WARN, HIGH); // Bật LED đỏ báo động
        Serial.println("--> [ALERT] AI Forecast: Severe Pollution Expected! "
                       "Buzzer and LED activated.");
        buzzer_active = true;
      } else {
        if (buzzer_active) {
          noTone(PIN_BUZZER);
          buzzer_active = false;
        }
        digitalWrite(PIN_LED_WARN, LOW);
      }
      // -----------------------------------------------------

// --- 5. GỬI DỮ LIỆU LÊN FIREBASE ---
#if USE_FIREBASE
      static unsigned long lastFirebaseCurrentUpdate = 0;
      static unsigned long lastFirebaseHistoryUpdate = 0;
      bool pushHistory = false;

      // Đẩy dữ liệu vào lịch sử định kỳ mỗi 60 giây (1 phút)
      if (millis() - lastFirebaseHistoryUpdate > 60000 ||
          lastFirebaseHistoryUpdate == 0) {
        pushHistory = true;
        lastFirebaseHistoryUpdate = millis();
      }

      // Cập nhật dữ liệu realtime hiện tại mỗi 5 giây
      if (millis() - lastFirebaseCurrentUpdate > 5000) {
        firebaseManager.sendData(raw_pm25, data.temperature, data.humidity,
                                 data.pressure, data.gasVoltage,
                                 data.dustDensity, final_pm25, pushHistory);
        lastFirebaseCurrentUpdate = millis();
      }
#endif
      // -----------------------------------

    } else {
      Serial.println("AI inference failed!");
    }
  }

  delay(5000); // Read every 5 seconds for now
}
