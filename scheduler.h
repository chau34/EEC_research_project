/* Scheduler
 *
 * Two modes: preemptive and cooperative
 *
 * Created on 10/25/2026 for research purposes
 */

// TODO: energy and time between switching modes??

#pragma once
#include <queue>
#include <memory>
#include <array>
#include "event.h"
#include "energy_params.h"

extern std::queue<int> ready_queue;

#define TIME_QUANTUM (10 * TICK_US)

/* This will never be called in a cooperative scheduler
 * as TIME_QUANTUM will be infinite */
void timerInterrupt();
void schedule();

/* Puts process in ready queue of scheduler */
void processArrives(int pid);

/* Removes process forever, should be the process currently running */
bool processExits(int pid);