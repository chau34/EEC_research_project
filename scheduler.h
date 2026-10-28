/* Simulator
 *
 * Two modes: preemptive and cooperative
 * Preemptive scheduler can have different selection algos (RR, priority, etc.)
 *
 * Created on 10/25/2026 for research purposes
 */

// TODO: energy and time between switching modes??

#pragma once
#include <queue>
#include <memory>
#include "process.h"

extern std::queue<int> ready_queue;
extern std::shared_ptr<Process> current_process;

#define TIME_QUANTUM 10

/* This will never be called in a cooperative scheduler
 * as TIME_QUANTUM will be infinite */
void timerInterrupt();
void schedule();

/* Puts process in ready queue of scheduler */
void processArrives(int pid);

/* Removes process forever, should be the process currently running */
void processExits(int pid);