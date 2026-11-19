/* Core
 *
 * Contains core metadata and energy calculations
 * 
 * Created on 10/25/2026 for research purposes
 */

// TODO: C states and P states? Energy Delay Product? Dynamic vs Static Energy?
// caches, core affinity, thermal scheduling/throttling

#pragma once

#include <cstdint>
#include <memory>
#include "process.h"

#define NUM_CORES 8

// TODO: how many c_states do we want, I think 5 is sufficient
enum C_STATE {
    C0,     // running at max
    C1,     // idle
    C3,     // light sleep, cache is flushed
    C6,     // deep sleep, core uses almost no energy
    C8,     // turn off, only used when all cores are at C6
};

// TODO: is this necessary?
enum P_STATE {
    P0,
    P1,
    P2,
    P3
};

struct Core {
    int core_num;
    std::uint64_t total_energy;
    C_STATE c_state;
    P_STATE p_state;
    std::uint64_t last_c_change;        // used to calculate energy used by c_state at interrupts
    std::uint64_t last_p_change;        // used to calculate energy used by p_state at interrupts
    std::uint64_t last_cache_flush;
    std::shared_ptr<Process> running;
    int last_pid;
    bool context_switching;
    bool changing_c_state;
    std::uint64_t last_idle;       // keeps track of last time idle, corresponds with c-state step down
    bool timer_interrupted;
};

extern std::shared_ptr<Core> cores[NUM_CORES];

void initCores();

/* Called during interrupts to calculate energy */
void calculateEnergy();

/* Calculate energy caused by changing states */
void changePState(int core);
void changeCState(int core);