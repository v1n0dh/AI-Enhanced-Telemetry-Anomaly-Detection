import time
import numpy as np
from sklearn.ensemble import IsolationForest
from sklearn.preprocessing import StandardScaler

# Paths to /sysfs telemetry entries
CPU_TEMP_PATH = "/sys/kernel/telemetry/cpu_temp"
BATTERY_PATH = "/sys/kernel/telemetry/battery_level"
NETWORK_PATH = "/sys/kernel/telemetry/network_throughput"

# Read telemetry from sysfs
def read_telemetry():
    try:
        with open(CPU_TEMP_PATH) as f:
            cpu = int(f.read().strip())
        with open(BATTERY_PATH) as f:
            battery = int(f.read().strip())
        with open(NETWORK_PATH) as f:
            net = int(f.read().strip())
        return [cpu, battery, net]
    except Exception as e:
        print("Error reading telemetry:", e)
        return None

# Simulate initial training data (collect normal telemetry patterns)
train_data = []
print("Collecting initial telemetry samples for training...")
for _ in range(100):
    sample = read_telemetry()
    if sample:
        train_data.append(sample)
    time.sleep(0.5)  # collect samples over 50 seconds

train_data = np.array(train_data)

# Scale the telemetry data
scaler = StandardScaler()
train_data_scaled = scaler.fit_transform(train_data)

# Train Isolation Forest
model = IsolationForest(contamination=0.05, random_state=42)
model.fit(train_data_scaled)
print("Isolation Forest model trained on live telemetry.")

# Real-time monitoring
print("Starting real-time telemetry anomaly detection...")
while True:
    telemetry = read_telemetry()
    if telemetry:
        telemetry_scaled = scaler.transform([telemetry])
        prediction = model.predict(telemetry_scaled)
        # -1 means anomaly, 1 means normal
        if prediction[0] == -1:
            print(f"[ALERT] Anomaly detected! Telemetry: {telemetry}")
        else:
            print(f"[OK] Normal: {telemetry}")
    time.sleep(2)  # match the kernel module update interval
