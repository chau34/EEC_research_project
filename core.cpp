#include "core.h"
#include "event.h"

std::shared_ptr<Core> cores[NUM_CORES];

void initCores() {
    for (int i = 0; i < NUM_CORES; i++)
        cores[i] = std::make_shared<Core>(
            Core{i, 0, C0, P0, 0, 0, 0, NULL, 0, false, false});
}

void calculateEnergy() {
    
}

void changePState(int core, P_STATE p_state) {
    cores[core]->p_state = p_state;
    // TODO: energy

    cores[core]->last_p_change = real_time;
}

void changeCState(int core, C_STATE c_state) {
    cores[core]->c_state = c_state;
    if (c_state == C3)
        cores[core]->last_cache_flush = real_time;
    // TODO: energy

    cores[core]->last_c_change = real_time;
}