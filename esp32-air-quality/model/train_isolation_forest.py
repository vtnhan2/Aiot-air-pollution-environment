import os
import numpy as np
from sklearn.ensemble import IsolationForest

# --- 1. SIMULATE SENSOR DATA ---
# In a real scenario, you'd use historically collected clean data.
# Here we simulate normal data and some anomalies for training.
print("Generating simulated data for Isolation Forest...")
np.random.seed(42)

# Features: [Temperature, Humidity, Pressure, CO2, Dust, GasVoltage]
# Normal conditions
normal_temp = np.random.normal(25, 2, 1000)
normal_hum = np.random.normal(50, 10, 1000)
normal_pres = np.random.normal(1013, 5, 1000)
normal_co2 = np.random.normal(400, 50, 1000)
normal_dust = np.random.normal(15, 5, 1000)
normal_gas = np.random.normal(0.5, 0.1, 1000)

X_normal = np.column_stack((normal_temp, normal_hum, normal_pres, normal_co2, normal_dust, normal_gas))

# Anomalous conditions (e.g., sudden fire or extreme pollution)
anomaly_temp = np.random.normal(40, 5, 50)
anomaly_hum = np.random.normal(20, 5, 50)
anomaly_pres = np.random.normal(1000, 5, 50)
anomaly_co2 = np.random.normal(2000, 200, 50)
anomaly_dust = np.random.normal(150, 30, 50)
anomaly_gas = np.random.normal(2.5, 0.5, 50)

X_anomaly = np.column_stack((anomaly_temp, anomaly_hum, anomaly_pres, anomaly_co2, anomaly_dust, anomaly_gas))

X_train = np.vstack((X_normal, X_anomaly))

# --- 2. TRAIN ISOLATION FOREST ---
print("Training Isolation Forest...")
clf = IsolationForest(n_estimators=100, contamination=0.05, random_state=42)
clf.fit(X_train)

# --- 3. EXTRACT RULES / THRESHOLDS FOR ESP32 ---
# While we can export the full Isolation Forest trees (using emlearn or micromlgen),
# for ESP32-S3, a simpler approach for Phase 1 Anomaly Detection is using Statistical Thresholds (Z-Score/IQR) 
# derived from this data, as it uses <1KB RAM and is extremely fast.

# Calculate IQR (Interquartile Range) for normal data
q1 = np.percentile(X_normal, 25, axis=0)
q3 = np.percentile(X_normal, 75, axis=0)
iqr = q3 - q1

# Define acceptable bounds (1.5 * IQR is standard, we use 3 for extreme anomalies to avoid false positives)
lower_bound = q1 - 3 * iqr
upper_bound = q3 + 3 * iqr

print("Exporting statistical anomaly thresholds to C header...")

c_code = f"""// Auto-generated Anomaly Detection Thresholds (IQR Method)
// Features order: Temp, Hum, Pres, CO2, Dust, Gas

#ifndef ANOMALY_THRESHOLDS_H
#define ANOMALY_THRESHOLDS_H

const float THRESHOLD_LOWER[6] = {{ {', '.join(map(lambda x: f"{x:.2f}f", lower_bound))} }};
const float THRESHOLD_UPPER[6] = {{ {', '.join(map(lambda x: f"{x:.2f}f", upper_bound))} }};

#endif // ANOMALY_THRESHOLDS_H
"""

output_path = os.path.join(os.path.dirname(__file__), "..", "src", "ai", "anomaly_thresholds.h")
os.makedirs(os.path.dirname(output_path), exist_ok=True)

with open(output_path, "w") as f:
    f.write(c_code)

print(f"Done! Thresholds saved to {output_path}")
