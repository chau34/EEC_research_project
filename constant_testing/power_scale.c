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
double cov(double* power_values) {
    double x_mean;
    double y_mean;
    double x = 0;
    double y = 0;
    for (int i = 0; i <= NUM_CPUS; i++) {
        x += power_values[i];
        y += i;
    }
    x_mean = x / (NUM_CPUS + 1);
    y_mean = y / (NUM_CPUS + 1);

    double sum = 0;
    for (int i = 0; i <= NUM_CPUS; i++) {
        sum += (power_values[i] - x_mean) * (i - y_mean);
    }
    return sum / (NUM_CPUS + 1);
}

double variance(int k) {
    double average = 0;
    for (int i = 0; i < k; i++) {
        average += i;
    }
    average /= k;
    double sum = 0;
    for (int i = 0; i < k; i++) {
        double value = i - average;
        sum += value * value;
    }
    return sum / k;
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
    for (int i = 0; i < NUM_CPUS; i++) {
        cpus[i] = i + 1;
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

    double cov_value = cov(power_values);
    double variance_value = variance(NUM_CPUS + 1);
    double p_active = cov_value / variance_value;
    double p_mean = 0;
    double k_mean = 0;
    for (int i = 0; i < NUM_CPUS + 1; i++) {
        p_mean += power_values[i];
        k_mean += i;
    }
    p_mean /= NUM_CPUS + 1;
    k_mean /= NUM_CPUS + 1;
    double p_shared = p_mean - (p_active * k_mean);
    
    printf("cov: %.0f\n", cov_value);
    printf("variance: %.0f\n", variance_value);
    printf("mean power: %.0f\n", p_mean);
    printf("mean k: %.0f\n", k_mean);

    printf("P_active = %.0f\n", p_active);
    printf("P_shared = %.0f\n", p_shared);
    printf("Shared Idle = %.0f\n", power_values[0]);
    return 0;
}
