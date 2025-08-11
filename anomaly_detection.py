import pandas as pd
from sklearn.ensemble import IsolationForest
import numpy as np
import time
import posix_ipc
import mmap
import struct

# Load training data and train model
df = pd.read_csv("telemetry.csv")
model = IsolationForest(contamination=0.05, random_state=42)
model.fit(df)
print("✅ Model trained. Starting real-time inference...")

# Shared memory setup - must match telemetry.c SHM_NAME and struct format
SHM_NAME = "/telemetry_shm"
DATA_FORMAT = "fff"  # float cpu_temp, float battery, float net_mbps
DATA_SIZE = struct.calcsize(DATA_FORMAT)

# Open existing shared memory
shm = posix_ipc.SharedMemory(SHM_NAME)
mapfile = mmap.mmap(shm.fd, DATA_SIZE, mmap.MAP_SHARED, mmap.PROT_READ)
shm.close_fd()

try:
    while True:
        mapfile.seek(0)
        data = mapfile.read(DATA_SIZE)
        cpu_temp, battery, net_mbps = struct.unpack(DATA_FORMAT, data)

        reading = np.array([[cpu_temp, battery, net_mbps]])
        prediction = model.predict(reading)  # 1 = normal, -1 = anomaly

        status = "⚠️ Anomaly" if prediction[0] == -1 else "OK"
        print(f"Reading: CPU={cpu_temp:.2f}°C | Battery={battery:.2f}% | Net={net_mbps:.2f} Mbps | Status: {status}")

        time.sleep(0.5)

except KeyboardInterrupt:
    print("Stopping real-time inference...")

finally:
    mapfile.close()

