import os
import pandas as pd
import numpy as np
import tensorflow as tf
from sklearn.preprocessing import MinMaxScaler
import urllib.request
import zipfile

# --- 1. CONFIGURATION ---
SEQ_LENGTH = 24  # Use past 24 hours to predict
PREDICT_AHEAD = 1 # Predict 1 hour ahead
BATCH_SIZE = 64
EPOCHS = 10
DATA_URL = "https://archive.ics.uci.edu/ml/machine-learning-databases/00381/PRSA_data_2010.1.1-2014.12.31.csv"
DATA_FILE = "PRSA_data.csv"
MODEL_NAME = "pm25_lstm"

# --- 2. DOWNLOAD & LOAD DATA ---
if not os.path.exists(DATA_FILE):
    print("Downloading Beijing PM2.5 dataset...")
    urllib.request.urlretrieve(DATA_URL, DATA_FILE)

print("Loading data...")
df = pd.read_csv(DATA_FILE)

# Drop rows with NaN in PM2.5
df = df.dropna(subset=['pm2.5'])

# Select features: PM2.5, DEWP (Dew Point / Humidity proxy), TEMP, PRES
features = ['pm2.5', 'DEWP', 'TEMP', 'PRES']
data = df[features].values

# --- 3. PREPROCESSING ---
print("Scaling data...")
scaler = MinMaxScaler(feature_range=(0, 1))
scaled_data = scaler.fit_transform(data)

# Create sequences
def create_sequences(dataset, seq_length, predict_ahead):
    X, y = [], []
    for i in range(len(dataset) - seq_length - predict_ahead + 1):
        X.append(dataset[i:(i + seq_length)])
        y.append(dataset[i + seq_length + predict_ahead - 1, 0]) # 0 is pm2.5 index
    return np.array(X), np.array(y)

X, y = create_sequences(scaled_data, SEQ_LENGTH, PREDICT_AHEAD)

# Split train/test (80/20)
split = int(0.8 * len(X))
X_train, X_test = X[:split], X[split:]
y_train, y_test = y[:split], y[split:]

# --- 4. BUILD & TRAIN LSTM MODEL ---
print("Building LSTM model...")
model = tf.keras.Sequential([
    tf.keras.layers.LSTM(64, return_sequences=True, unroll=True, input_shape=(SEQ_LENGTH, len(features))),
    tf.keras.layers.LSTM(32, return_sequences=False, unroll=True),
    tf.keras.layers.Dense(16, activation='relu'),
    tf.keras.layers.Dense(1)
])

model.compile(optimizer='adam', loss='mse', metrics=['mae'])

print("Training model...")
model.fit(X_train, y_train, batch_size=BATCH_SIZE, epochs=EPOCHS, validation_split=0.1)

# --- 5. TFLITE CONVERSION & QUANTIZATION ---
print("Converting to TFLite (INT8)...")

def representative_data_gen():
    for i in range(100):
        # Provide float32 data for quantization calibration
        data_sample = np.expand_dims(X_train[i], axis=0).astype(np.float32)
        yield [data_sample]

converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]
converter.representative_dataset = representative_data_gen
converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
converter.inference_input_type = tf.int8
converter.inference_output_type = tf.int8

tflite_model_quant = converter.convert()

# Save TFLite model
with open(f"{MODEL_NAME}_quant.tflite", "wb") as f:
    f.write(tflite_model_quant)

# --- 6. EXPORT TO C HEADER (.h) ---
print("Exporting to C header file...")
hex_array = ', '.join([f'0x{byte:02x}' for byte in tflite_model_quant])

c_code = f"""// Auto-generated TFLite Micro Model
// Features: PM2.5, DEWP, TEMP, PRES
// Sequence Length: {SEQ_LENGTH}
// Predict Ahead: {PREDICT_AHEAD}h

#ifndef MODEL_DATA_H
#define MODEL_DATA_H

#include <stdint.h>

const unsigned int model_data_len = {len(tflite_model_quant)};
const unsigned char model_data[] = {{
    {hex_array}
}};

#endif // MODEL_DATA_H
"""

output_path = os.path.join(os.path.dirname(__file__), "..", "src", "ai", "model_data.h")
os.makedirs(os.path.dirname(output_path), exist_ok=True)

with open(output_path, "w") as f:
    f.write(c_code)

print(f"Done! Model saved to {output_path}")
