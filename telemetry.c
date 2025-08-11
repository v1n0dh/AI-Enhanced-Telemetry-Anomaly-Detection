#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>

#define SHM_NAME "/telemetry_shm"

typedef struct {
    float battery_voltage;
    float battery_current;
    float temperature;
    unsigned long timestamp;
} TelemetryData;

// Reads the first line from a file
char *readfile(const char *path) {
    static char buf[128];
    FILE *fp = fopen(path, "r");
    if (!fp) return NULL;
    if (!fgets(buf, sizeof(buf), fp)) {
        fclose(fp);
        return NULL;
    }
    fclose(fp);
    buf[strcspn(buf, "\n")] = 0; // strip newline
    return buf;
}

// CPU temperature in °C
float read_cpu_temperature() {
    char *temp_str = readfile("/sys/class/thermal/thermal_zone0/temp");
    if (!temp_str) return -1;
    return atof(temp_str) / 1000.0; // Convert from millidegrees
}

// Battery percentage
float read_battery_percentage() {
    char *cap_str = readfile("/sys/class/power_supply/BAT0/capacity");
    if (!cap_str) return -1;
    return atof(cap_str);
}

// Network throughput (KB/s) — requires state across calls
float read_network_throughput() {
    static long rx_old = 0, tx_old = 0;
    static int first_call = 1;

    char path_rx[128], path_tx[128];
    snprintf(path_rx, sizeof(path_rx), "/sys/class/net/wlp1s0/statistics/rx_bytes");
    snprintf(path_tx, sizeof(path_tx), "/sys/class/net/wlp1s0/statistics/tx_bytes");

    char *rx_str = readfile(path_rx);
    char *tx_str = readfile(path_tx);
    if (!rx_str || !tx_str) return -1;

    long rx = atol(rx_str);
    long tx = atol(tx_str);

    if (first_call) {
        rx_old = rx;
        tx_old = tx;
        first_call = 0;
        return 0.0;
    }

    long rx_diff = rx - rx_old;
    long tx_diff = tx - tx_old;
    rx_old = rx;
    tx_old = tx;

    // Convert bytes/sec to kilobytes/sec
    return (rx_diff + tx_diff) / 1024.0;
}

int main() {
	srand(time(NULL));

	int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }

	if (ftruncate(shm_fd, sizeof(TelemetryData)) == -1) {
        perror("ftruncate");
        return 1;
    }

    FILE *fp = fopen("telemetry.csv", "w");
    if (!fp) {
        perror("Failed to open telemetry.csv");
        return 1;
    }

	// Write CSV header if file empty
    fseek(csv_fp, 0, SEEK_END);
    long size = ftell(csv_fp);
    if (size == 0) {
        fprintf(csv_fp, "battery_voltage,battery_current,temperature,timestamp\n");
        fflush(csv_fp);
    }

	TelemetryData *shared_data = mmap(NULL, sizeof(TelemetryData),
                                      PROT_READ | PROT_WRITE, MAP_SHARED,
                                      shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    printf("Telemetry producer started. Writing to shared memory...\n");

    fprintf(fp, "cpu_temp_c,battery_pct,net_kbps\n");

	while (1) {
			float battery_voltage = read_battery_voltage();
			float battery_current = read_battery_current();
			float temperature = read_temperature();
			unsigned long timestamp = (unsigned long)time(NULL);

			// Write to shared memory
			shared_data->battery_voltage = battery_voltage;
			shared_data->battery_current = battery_current;
			shared_data->temperature = temperature;
			shared_data->timestamp = timestamp;

			// Also append to CSV file
			fprintf(csv_fp, "%.2f,%.2f,%.2f,%lu\n",
					battery_voltage, battery_current, temperature, timestamp);
			fflush(csv_fp);

			printf("[Telemetry] V=%.2fV, I=%.2fA, Temp=%.2fC, Time=%lu\n",
				   battery_voltage, battery_current, temperature, timestamp);

			sleep(1);
	}

	munmap(shared_data, sizeof(TelemetryData));
    close(shm_fd);
    shm_unlink(SHM_NAME);

    fclose(fp);

    return 0;
}
