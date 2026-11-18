#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sched.h>
#include <time.h>
#include "testing_helpers.h"

typedef struct {
    char energy[256];
    double wrap_uj;
} Zone;

static int find_zone(int socket, Zone *z) {
    char dir[160];
    char p[320];
    char name[64];
    char b[64];

    snprintf(dir, sizeof dir, "/sys/class/powercap/intel-rapl:%d", socket);

    snprintf(p, sizeof p, "%s/name", dir);
    if (read_into_buf(p, name, sizeof name) || strncmp(name, "package", 7))
        return -1;

    snprintf(z->energy, sizeof z->energy, "%s/energy_uj", dir);

    snprintf(p, sizeof p, "%s/max_energy_range_uj", dir);
    z->wrap_uj = read_into_buf(p, b, sizeof b) ? 0 : atof(b);

    return 0;
}

static double read_energy(const Zone *z) {
    char b[64];
    if (read_into_buf(z->energy, b, sizeof b)) {
        printf("Energy read failed");
        exit(1);
    }
    return atof(b);
}

static double get_energy(const Zone *z, double a, double b) {
    return b >= a ? b - a : b + z->wrap_uj - a;
}
