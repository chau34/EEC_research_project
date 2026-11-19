#define _GNU_SOURCE

#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "testing_helpers.h"

int main(int argc, char **argv) {
    if (argc != 4) {
        printf("Usage: [cpu] [seconds] [threshold_ns]\n");
        return 1;
    }

    int cpu = atoi(argv[1]);
    int seconds = atoi(argv[2]);
    int threshold_ns = atoll(argv[3]);
    long long duration_ns = (long long) seconds * 1000000000LL;

    pin_cpu(cpu);

    // Measurement variables
    long long total_interrupt_ns = 0;
    long long interrupts = 0;

    long long start = now_ns();
    long long prev = start;

    // Measure timestamp gaps
    while (1) {
        long long curr = now_ns();

        if (curr - start >= duration_ns)
            break;

        long long interval = curr - prev;

        if (interval > threshold_ns) {
            total_interrupt_ns += interval;
            interrupts++;
        }

        prev = curr;
    }

    double avg_interrupt_ns = interrupts > 0 ? (double) total_interrupt_ns / interrupts : 0.0;
    printf("Average Interrupt Duration: %.2f ns\n", avg_interrupt_ns);
    return 0;
}
