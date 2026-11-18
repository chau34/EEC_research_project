#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "testing_helpers.h"

typedef struct Line {
    struct Line *next;
    char padding[CACHE_LINE_SIZE - sizeof(void *)];
} Line;

static volatile Line *keep;

Line *make_working_set(size_t n) {
    Line *lines = aligned_alloc(CACHE_LINE_SIZE, n * sizeof(Line));
    size_t *order = malloc(n * sizeof(size_t));

    if (!lines || !order) {
        perror("malloc");
        exit(1);
    }

    for (size_t i = 0; i < n; i++)
        order[i] = i;

    for (size_t i = n - 1; i > 0; i--) {
        size_t j = rand() % (i + 1);
        size_t tmp = order[i];
        order[i] = order[j];
        order[j] = tmp;
    }

    for (size_t i = 0; i < n; i++)
        lines[order[i]].next = &lines[order[(i + 1) % n]];

    free(order);
    return lines;
}

void chase(Line *p, size_t n) {
    for (size_t i = 0; i < n; i++)
        p = p->next;
    keep = p;
}

double timed_chase(Line *p, size_t n) {
    double start = now_ns();
    chase(p, n);
    return now_ns() - start;
}

void evict_cache(char *buf, size_t bytes) {
    volatile char *p = buf;
    for (size_t i = 0; i < bytes; i += CACHE_LINE_SIZE)
        p[i]++;
}

int cmp(const void *a, const void *b) {
    double x = *(double *)a, y = *(double *)b;
    return (x > y) - (x < y);
}

double median(double *array, int size) {
    qsort(array, size, sizeof(double), cmp);
    return array[size / 2];
}

int main(int argc, char **argv) {
    if (argc < 4 || argc > 6) {
        printf("Usage: [cpu1] [cpu2] [working set (KB)] [evict (MB)] [trials]\n");
        return 1;
    }

    int cpu1 = atoi(argv[1]);
    int cpu2 = atoi(argv[2]);
    size_t kb = atoi(argv[3]);
    size_t mb = atoi(argv[4]);
    int trials = atoi(argv[5]);

    size_t n = kb * 1024 / CACHE_LINE_SIZE;
    size_t bytes = mb * 1024 * 1024;

    srand(time(NULL));

    Line *ws = make_working_set(n);

    char *eviction = aligned_alloc(CACHE_LINE_SIZE, bytes);

    for (size_t i = 0; i < bytes; i++)
        eviction[i] = 0;

    double *initial = malloc(trials * sizeof(double));
    double *move = malloc(trials * sizeof(double));
    double *migrated = malloc(trials * sizeof(double));
    double *evicted = malloc(trials * sizeof(double));
    for (int t = 0; t < trials; t++) {
        pin_cpu(cpu1);
        chase(ws, n);
        chase(ws, n);
        initial[t] = timed_chase(ws, n);

        chase(ws, n);
        double start = now_ns();
        pin_cpu(cpu2);
        move[t] = now_ns() - start;
        migrated[t] = timed_chase(ws, n);
        pin_cpu(cpu1);
        chase(ws, n);
        evict_cache(eviction, bytes);
        evicted[t] = timed_chase(ws, n);
    }

    printf("Initial traversal: %.2f ns\n", median(initial, trials));
    printf("Migration call: %.2f ns\n", median(move, trials));
    printf("Migrated traversal: %.2f ns\n", median(migrated, trials));
    printf("Post-eviction traversal: %.2f ns\n", median(evicted, trials));
    printf("Migration cache penalty: %.2f ns\n", median(migrated, trials) - median(initial, trials));
    printf("Total migration overhead: %.2f ns\n", median(move, trials) + median(migrated, trials) - median(initial, trials));
    printf("Eviction penalty: %.2f ns\n", median(evicted, trials) - median(initial, trials));
    return 0;
}
