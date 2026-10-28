/* The main program that gets compiled and run
 *
 * Created on 10/25/2026 for research purposes
 */

#include "event.h"
#include <string>

int main(int argc, char* argv[]) {
    if (argc != 2)
        return -1;

    generateWorkload(std::stoi(argv[1]));
    startSimulation();
    return 0;
}