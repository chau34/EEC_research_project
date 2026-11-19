#pragma once
#include <time.h>
#include <sched.h>

#define CACHE_LINE_SIZE 64

#define L1_CACHE_SIZE (32 * 1024)
#define L2_CACHE_SIZE (1024 * 1024)
#define L3_CACHE_SIZE (22 * 1024 * 1024)

#define DEFAULT_EVICT_MB 128
#define DEFAULT_TRIALS 200


static double now_ns(void) {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e9 + t.tv_nsec;
}

static int read_into_buf(const char *path, char *buf, size_t n) {
    FILE *f = fopen(path, "r");
    if (!f)
        return -1;
    if (!fgets(buf, (int) n, f)) {
        fclose(f);
        return -1;
    }
    fclose(f); return 0;
}

/* Pins current process to specified CPU (cpu) */
static void pin_cpu(int cpu) {
    cpu_set_t set;

    CPU_ZERO(&set);
    CPU_SET(cpu, &set);

    if (sched_setaffinity(0, sizeof(set), &set) == -1) {
        perror("sched_setaffinity");
        exit(EXIT_FAILURE);
    }
}