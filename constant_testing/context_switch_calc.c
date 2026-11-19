#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include "testing_helpers.h"

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Improper arguments. Expected: [cpu] [round] [trials]");
    }

    // Extract parameters
    int cpu = atoi(argv[1]);
    long rounds = atol(argv[2]);
    int trials = atoi(argv[3]);
    
    pin_cpu(cpu);

    char c = 'x';
    // Find syscall cost without context switch
    int s[2];
    pipe(s);
    double time_start = now_ns();
    for (long i = 0; i < rounds; i++) {
        write(s[1], &c, 1);
        read(s[0], &c, 1);
    }
    double time_per_wr = (now_ns() - time_start) / rounds;

    // Context switch between processes
    int a[2];
    int b[2];
    pipe(a);
    pipe(b);
    pid_t pid = fork();
    // Child running
    if (pid == 0) {
        // Reads from pipe a and writes to pipe b
        while (read(a[0], &c, 1) == 1)
            write(b[1], &c, 1);
        _exit(0);
    }

    for (int i = 0; i < trials; i++) {
        struct rusage r0;
        struct rusage r1;
        getrusage(RUSAGE_SELF, &r0);
        
        double time = now_ns();
        for (long i = 0; i < rounds; i++) {
            write(a[1], &c, 1); read(b[0], &c, 1);
        }
        double round_trip_latency = (now_ns() - time) / rounds;
        getrusage(RUSAGE_SELF, &r1);
        double trial_results = (round_trip_latency - 2 * time_per_wr) / 2;
        printf("trial %d: total time %.0f ns switch time %.0f ns switches per round: %.2f\n",
               i, round_trip_latency, trial_results, (double) (r1.ru_nvcsw - r0.ru_nvcsw) / rounds);
    }
    close(a[1]);
    return 0;
}