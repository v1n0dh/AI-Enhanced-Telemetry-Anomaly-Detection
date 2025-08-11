import pandas as pd
from sklearn.ensemble import IsolationForest
import numpy as np
import time

# Load data
df = pd.read_csv("telemetry.csv")

# Train Isolation Forest
model = IsolationForest(contamination=0.05, random_state=42)
model.fit(df)

print("✅ Model trained. Starting real-time inference...")

# Simulated streaming telemetry
while True:
    # Simulated new readings (replace with real firmware IPC/serial data in production)
    cpu_temp = np.random.uniform(50, 60)
    battery = np.random.uniform(78, 82)
    net_mbps = np.random.uniform(50, 65)

    reading = np.array([[cpu_temp, battery, net_mbps]])
    prediction = model.predict(reading)  # 1 = normal, -1 = anomaly

    print(f"Reading: CPU={cpu_temp:.2f}°C | Battery={battery:.2f}% | Net={net_mbps:.2f} Mbps",
          "| Status:", "⚠️ Anomaly" if prediction[0] == -1 else "OK")

    time.sleep(0.5)
