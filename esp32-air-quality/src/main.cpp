#include <Arduino.h>
#include <WiFi.h>
#include "sensors/sensor_manager.h"
#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "ai/model_data.h"
// WiFi Credentials
const char* ssid = "Ruby";
const char* password = "79797979";

SensorManager sensorManager;

// TFLite Globals
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;

// Feature scaling constants (MinMaxScaler matching Python training config)
const float FEATURE_MIN[4] = {0.0f, -40.0f, -20.0f, 990.0f};
const float FEATURE_MAX[4] = {1000.0f, 30.0f, 45.0f, 1050.0f};

// Helper function to scale raw data to [0, 1]
float scaleData(float value, int feature_index) {
    if (isnan(value)) value = FEATURE_MIN[feature_index]; // Default to MIN if NaN
    if (value < FEATURE_MIN[feature_index]) value = FEATURE_MIN[feature_index];
    if (value > FEATURE_MAX[feature_index]) value = FEATURE_MAX[feature_index];
    return (value - FEATURE_MIN[feature_index]) / (FEATURE_MAX[feature_index] - FEATURE_MIN[feature_index]);
}

// Helper function to inverse scale [0, 1] back to raw value
float inverseScale(float scaled_value, int feature_index) {
    return scaled_value * (FEATURE_MAX[feature_index] - FEATURE_MIN[feature_index]) + FEATURE_MIN[feature_index];
}

// Allocate memory for the tensor arena
constexpr int kTensorArenaSize = 2 * 1024 * 1024; // 2MB in PSRAM for LSTM and weights
// Put arena in PSRAM if available, otherwise use normal RAM
uint8_t* tensor_arena = nullptr;

// Circular buffer for 24 hours (we use simulated quick steps here for testing)
int8_t input_buffer[24][4] = {0};
int step_count = 0;
void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n--- AIoT Air Quality System Starting ---");
    
    // Connect to WiFi
    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Check if PSRAM is initialized
    if (psramFound()) {
        Serial.printf("PSRAM initialized successfully. Size: %d bytes\n", ESP.getPsramSize());
    } else {
        Serial.println("Warning: PSRAM not found!");
    }

    sensorManager.begin();
    
    Serial.println("\n--- Initializing AI Model (TFLite) ---");
    tflite::InitializeTarget();
    
    // Allocate arena in PSRAM
    tensor_arena = (uint8_t*) heap_caps_malloc(kTensorArenaSize, MALLOC_CAP_SPIRAM);
    if (!tensor_arena) {
        Serial.println("Failed to allocate tensor arena in PSRAM! Trying Internal RAM...");
        tensor_arena = (uint8_t*) malloc(kTensorArenaSize);
    }
    
    model = tflite::GetModel(model_data);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        Serial.printf("Model schema mismatch! Expected %d but got %d\n", TFLITE_SCHEMA_VERSION, model->version());
        return;
    }
    
    static tflite::AllOpsResolver resolver;
    static tflite::MicroErrorReporter micro_error_reporter;
    tflite::ErrorReporter* error_reporter = &micro_error_reporter;
    
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
    for (int i=0; i<input->dims->size; i++) {
        Serial.printf("%d%s", input->dims->data[i], (i == input->dims->size-1) ? "" : ", ");
    }
    Serial.println("]");
    
    Serial.println("System setup complete.");
}

void loop() {
    SensorData data = sensorManager.readAll();
    
    Serial.println("\n--- Sensor Readings ---");
    Serial.printf("Temperature: %.2f °C\n", data.temperature);
    Serial.printf("Humidity: %.2f %%\n", data.humidity);
    Serial.printf("Pressure: %.2f hPa\n", data.pressure);
    Serial.printf("CO2: %d ppm\n", data.co2);
    Serial.printf("Dust (PM2.5 approx): %.2f ug/m3\n", data.dustDensity);
    Serial.printf("Gas Voltage (MQ135): %.2f V\n", data.gasVoltage);
    
    // Check WiFi status in loop
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi Status: Connected");
    } else {
        Serial.println("WiFi Status: Disconnected");
    }
    
    // --- AI Inference ---
    if (input != nullptr) {
        // Shift buffer to the left (simulating sliding window of 24 timesteps)
        for(int i = 0; i < 23; i++) {
            for(int j = 0; j < 4; j++) {
                input_buffer[i][j] = input_buffer[i+1][j];
            }
        }
        
        // Add new reading to the end of the buffer
        // 1. Scale raw values to [0, 1] using MinMaxScaler
        float scaled_pm25 = scaleData(data.dustDensity, 0);
        float scaled_dewp = scaleData(data.humidity,    1); // Using humidity as DEWP proxy for now
        float scaled_temp = scaleData(data.temperature, 2);
        float scaled_pres = scaleData(data.pressure,    3);
        
        // 2. Quantize values to int8_t using TFLite tensor parameters
        input_buffer[23][0] = (int8_t)(scaled_pm25 / input->params.scale + input->params.zero_point);
        input_buffer[23][1] = (int8_t)(scaled_dewp / input->params.scale + input->params.zero_point);
        input_buffer[23][2] = (int8_t)(scaled_temp / input->params.scale + input->params.zero_point);
        input_buffer[23][3] = (int8_t)(scaled_pres / input->params.scale + input->params.zero_point);
        
        // Copy buffer to TFLite input tensor
        for(int i = 0; i < 24; i++) {
            for(int j = 0; j < 4; j++) {
                input->data.int8[i*4 + j] = input_buffer[i][j];
            }
        }
        
        // Run inference
        unsigned long start_time = millis();
        TfLiteStatus invoke_status = interpreter->Invoke();
        unsigned long inference_time = millis() - start_time;
        
        if (invoke_status == kTfLiteOk) {
            int8_t predicted_val = output->data.int8[0];
            
            // 3. Dequantize output back to [0, 1] float
            float dequantized_val = (predicted_val - output->params.zero_point) * output->params.scale;
            
            // 4. Inverse scale to get actual PM2.5 concentration (ug/m3)
            float final_pm25 = inverseScale(dequantized_val, 0);
            
            Serial.printf("--> AI Prediction (PM2.5 next hour): %.2f ug/m3\n", final_pm25);
            Serial.printf("--> Inference time: %lu ms\n", inference_time);
        } else {
            Serial.println("AI inference failed!");
        }
    }
    
    delay(5000); // Read every 5 seconds for now
}
