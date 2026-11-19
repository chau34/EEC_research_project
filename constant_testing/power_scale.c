#include "rapl.h"
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
// Change to whatever your stress-ng path is
#define STRESS_NG_PATH "~/EEC_research_project/src/stress-ng/stress-ng"
#define NUM_CPUS 16
#define NUM_WORKLOADS 3
static const char* METHODS[3] = {"int64", "float", "matrixprod"};

static pid_t start_worker(int cpu, const char* method) {
    pid_t pid = fork();
    if (pid < 0) {
        printf("Error, fork");
        return -1;
    }
    if (pid == 0) {
        pin_cpu(cpu);

        execl(STRESS_NG_PATH, "stress-ng", "--quiet", "--cpu", "1", 
              "--cpu-method", method, "--timeout", "1h",(char*) NULL);

        printf("Error, failed");
        exit(1);
    }
    return pid;
}

// Stop and reap all stress-ng processes.
static void stop_workers(pid_t *pids, int count) {
    // Terminate all workers.
    for (int i = 0; i < count; i++) {
        if (pids[i] > 0) {
            kill(pids[i], SIGTERM);
        }
    }

    // Wait for all workers to exit.
    for (int i = 0; i < count; i++) {
        if (pids[i] > 0) {
            while (waitpid(pids[i], NULL, 0) < 0 && errno == EINTR);
        }
    }
}

/* Returns population covariance for power values */
double cov(double *x, double *y, int n) {
    double x_mean = 0;
    double y_mean = 0;
    for (int i = 0; i < n; i++) {
        x_mean += x[i];
        y_mean += y[i];
    }
    x_mean /= n;
    y_mean /= n;
    double sum = 0;
    for (int i = 0; i < n; i++) {
        sum += (x[i] - x_mean) * (y[i] - y_mean);
    }
    return sum / n;
}

double variance(double *x, int n) {
    double mean = 0;
    for (int i = 0; i < n; i++) {
        mean += x[i];
    }
    mean /= n;
    double sum = 0;
    for (int i = 0; i < n; i++) {
        double diff = x[i] - mean;
        sum += diff * diff;
    }
    return sum / n;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Improper arguments. Expected: [control cpu] [seconds]\n");
        return 1;
    }

    // Pin the measurement process to the control CPU.
    pin_cpu(atoi(argv[1]));

    double secs = atof(argv[2]);

    int cpus[NUM_CPUS];
    int found = 0;
    int core_seen[NUM_CPUS];
    for (int c = 0; c < 256 && found < NUM_CPUS; c++) {
        char path[128];
        char b[32];
        snprintf(path, sizeof path, "/sys/devices/system/cpu/cpu%d/topology/physical_package_id", c);
        if (read_into_buf(path, b, sizeof b) || atoi(b) != 0)
            continue;
        snprintf(path, sizeof path, "/sys/devices/system/cpu/cpu%d/topology/core_id", c);
        if (read_into_buf(path, b, sizeof b))
            continue;
        int core = atoi(b), dup = 0;
        for (int i = 0; i < found; i++)
            if (core_seen[i] == core)
                dup = 1;
        if (dup)
            continue;
        core_seen[found] = core;
        cpus[found++] = c;
    }
    if (found < NUM_CPUS) {
        fprintf(stderr, "found only %d cores on package 0\n", found);
        return 1;
    }

    Zone pkg;

    if (find_zone(0, &pkg)) {
        fprintf(stderr, "no package-0 zone\n");
        return 1;
    }

    printf("k, pkg_mW\n");

    double power_values[NUM_CPUS + 1];

    // Increase the number of active CPU workers from 0 to n.
    for (int k = 0; k <= NUM_CPUS; k++) {
        
        pid_t pids[NUM_CPUS * NUM_WORKLOADS] = {0};
        int count = 0;
        // Start k workers.
        for (int i = 0; i < k; i++) {
            for (int j = 0; j < NUM_WORKLOADS; j++) {
                pids[count] = start_worker(cpus[i], METHODS[j]);

                if (pids[count] < 0) {
                    stop_workers(pids, count);
                    return 1;
                }
                count++;
            }
        }

        // Stabilize
        sleep(3);

        // Initial energy readings.
        double initial_energy = read_energy(&pkg);
        double start_time = now_ns() / 1e9;

        // Measurement interval.
        sleep((unsigned) secs);

        // Final energy readings.
        double final_energy = read_energy(&pkg);
        double end_time = now_ns() / 1e9;

        // Stop workers after measurement.
        stop_workers(pids, count);

        double time = end_time - start_time;

        // Calculate average power in milliwatts.
        double pkg_mW = get_energy(&pkg, initial_energy, final_energy) / time / 1e3;
        
        power_values[k] = pkg_mW;

        printf("%d, %.0f\n",k, pkg_mW);
        fflush(stdout);

        // Pause between configurations.
        sleep(2);
    }

    // Fall off after 14
    int n = 14;
    double cpu_counts[NUM_CPUS];

    double k_mean = 0;
    double p_mean = 0;
    for (int i = 0; i < n; i++) {
        cpu_counts[i] = i + 1;
        k_mean += cpu_counts[i];
        p_mean += power_values[i + 1];
    }
    k_mean /= n;
    p_mean /= n;
    double p_active = cov(cpu_counts, power_values + 1, n) / variance(cpu_counts, n);
    double p_shared = p_mean - p_active * k_mean;

    printf("mean k: %.1f\n", k_mean);
    printf("mean power: %.0f\n", p_mean);
    printf("P_active = %.0f\n", p_active);
    printf("P_shared = %.0f\n", p_shared);
    printf("Shared Idle = %.0f\n", power_values[0]);
    printf("Intercept = %.0f mW\n", p_shared - power_values[0]);
    return 0;
}
