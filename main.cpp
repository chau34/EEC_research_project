/* The main program that gets compiled and run
 *
 * Created on 10/25/2026 for research purposes
 */

#include "event.h"
#include <string>
#include <cstdio>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Format: sim [seed] [scheduler mode]");
        return -1;
    }

    generateWorkload(std::stoi(argv[1]));
    startSimulation(static_cast<SchedulerMode>(std::stoi(argv[2])));
    return 0;
}