#include "core.h"

std::shared_ptr<Core> cores[NUM_CORES];

void initCores() {
    for (int i = 0; i < NUM_CORES; i++)
        cores[i] = std::make_shared<Core>(Core{i, 0, C0, P0, 0, 0, 0, NULL});
}

void calculateEnergy() {
    
}

void changePState(int core) {

}

void changeCState(int core) {

}