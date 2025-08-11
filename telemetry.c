#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// Simulate reading from firmware-level interfaces like /sys/class or I2C
float read_cpu_temperature() {
    return 50.0 + (rand() % 1000) / 100.0; // ~50°C ± 10°C
}

float read_battery_percentage() {
    return 80.0 + (rand() % 400) / 100.0; // ~80% ± 4%
}

float read_network_throughput() {
    return 50.0 + (rand() % 3000) / 100.0; // ~50 Mbps ± 15 Mbps
}

int main() {
    FILE *fp = fopen("telemetry.csv", "w");
    if (!fp) {
        perror("Failed to open telemetry.csv");
        return 1;
    }

    srand(time(NULL));
    fprintf(fp, "cpu_temp,battery_pct,net_mbps\n");

    for (int i = 0; i < 200; i++) {
        float cpu_temp = read_cpu_temperature();
        float battery = read_battery_percentage();
        float net = read_network_throughput();

        fprintf(fp, "%.2f,%.2f,%.2f\n", cpu_temp, battery, net);
        fflush(fp);

        usleep(50000); // 50ms delay between samples
    }

    fclose(fp);
    return 0;
}
