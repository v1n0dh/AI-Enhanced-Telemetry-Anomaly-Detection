# AI-Enhanced Telemetry Anomaly Detection

This project demonstrates how machine learning can be used to enhance firmware telemetry diagnostics by detecting anomalies in hardware metrics in real-time.

## Architecture Overview

```text
+------------------------------------------------------+
| User-space (AI/ML Inference)                         |
| --------------------------------------------------   |
| • Python ML pipeline (scikit-learn: IsolationForest) |
| • Reads telemetry data from /sysfs                   |
| • Runs real-time anomaly detection                   |
| • Triggers alerts/logging                            |
+------------------------------------------------------+
                   ↑
                | (standard Linux interface: /sysfs)
                   ↓
+---------------------------------------------------------+
|   Kernel / Driver Layer                                 |
|---------------------------------------------------------|
|   • Custom kernel modules in C                          |
|   • Gather telemetry from hardware interfaces:          |
|       - I2C (battery stats)                             |
|       - CPU thermal sensors                             |
|       - Network stats                                   |
|   • Expose telemetry as files in /sysfs                 |
+---------------------------------------------------------+
                   ↑
                | (low-level hardware access)
                   ↓
+---------------------------------------------------------+
|   Hardware (Raspberry Pi)                               |
|   --------------------------------------------------    |
|   • CPU, sensors, I2C devices, network interface        |
|   • Acts as firmware platform for data acquisition      |
+---------------------------------------------------------+
```

## Features

- Collects **CPU temperature**, **battery stats**, and **network throughput** from a Raspberry Pi.
- Kernel module exposes telemetry through `/sys/kernel/telemetry/`.
- Python ML pipeline uses **Isolation Forest** to detect anomalies in real-time.
- Detects subtle anomalies beyond simple threshold-based monitoring.
- Provides logs and alerts when unusual behavior is detected.

---

## Getting Started

### Prerequisites

- Linux system (Raspberry Pi recommended)
- Kernel headers installed (`linux-headers-$(uname -r)`)
- Python 3.x
- Python packages: `scikit-learn`, `numpy`

### Build & Load Kernel Module

```bash
git clone https://github.com/v1n0dh/AI-Enhanced-Telemetry-Anomaly-Detection.git
cd AI-Enhanced-Telemetry-Anomaly-Detection

# Build kernel module
make

# Insert module
sudo insmod telemetry_module.ko

# Check kernel messages
dmesg | tail -n 10
```

### Access Telemetry Data

```bash
cat /sys/kernel/telemetry/cpu_temp
cat /sys/kernel/telemetry/battery_level
cat /sys/kernel/telemetry/network_throughput
```

### Run Python ML Pipeline

```bash
python3 telemetry_anomaly_live.py
```
- The pipeline collects initial normal telemetry samples to train the model.
-  Runs real-time anomaly detection.
-   Prints alerts when an anomaly is detected.

### Clean Up

```bash
sudo rmmod telemetry_module
make clean
```

