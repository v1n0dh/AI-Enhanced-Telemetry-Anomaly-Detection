#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

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
    FILE *fp = fopen("telemetry.csv", "w");
    if (!fp) {
        perror("Failed to open telemetry.csv");
        return 1;
    }

    fprintf(fp, "cpu_temp_c,battery_pct,net_kbps\n");

    for (int i = 0; i < 200; i++) {
        float cpu_temp = read_cpu_temperature();
        float battery = read_battery_percentage();
        float net_kbps = read_network_throughput();

        fprintf(fp, "%.2f,%.2f,%.2f\n", cpu_temp, battery, net_kbps);
        fflush(fp);

        usleep(1000000); // 1 second between samples
    }

    fclose(fp);
    return 0;
}
