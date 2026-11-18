/* The main program that gets compiled and run
 *
 * Created on 10/25/2026 for research purposes
 */

#include "event.h"
#include "workload.h"
#include <string>
#include <cstdio>

static void usage() {
    printf("Format: sim <input file> <scheduler mode> [seed offset] [--dump]\n"
           "  scheduler mode: 0 = preemptive, 1 = cooperative, 2 = hybrid\n"
           "  seed offset:    added to every class seed (default 0)\n"
           "  --dump:         also print one PROC line per process\n");
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        usage();
        return -1;
    }

    std::string input = argv[1];
    int mode;
    try {
        mode = std::stoi(argv[2]);
    } catch (...) {
        usage();
        return -1;
    }
    if (mode < 0 || mode > 2) {
        usage();
        return -1;
    }

    unsigned seed_offset = 0;
    bool dump = false;
    for (int i = 3; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--dump") {
            dump = true;
        } else {
            try {
                seed_offset = static_cast<unsigned>(std::stoul(arg));
            } catch (...) {
                usage();
                return -1;
            }
        }
    }

    generateWorkload(parseInput(input), seed_offset);
    setReportOptions(seed_offset, dump);
    startSimulation(static_cast<SchedulerMode>(mode));
    return 0;
}
