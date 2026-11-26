#include "scheduler.h"
#include "process.h"
#include "core.h"
#include <iostream>

std::queue<int> ready_queue;

/* Determine whether process wants to "borrow" time quantum
 * If yes, determine if it has enough credit */
void quantumBank(std::shared_ptr<Process> process, int core) {
    // check if future yield already scheduled, meaning this process just got to run
    if (process->yield_time > real_time)
        return;
    signed int borrowing = process->cache_hot_until - real_time;
    if (borrowing > 0) {
        if (process->quantum_credit >= borrowing) {
            borrowing = borrowing > TIME_QUANTUM ? TIME_QUANTUM : borrowing;
            process->quantum_credit -= borrowing;
            addYieldEvent(process->PID, borrowing);
        } else {
            int pid = ready_queue.front();
            ready_queue.pop();
            ready_queue.push(process->PID);
            contextSwitchOut(core);
            contextSwitchIn(core, pid);
        }
    }
}

/* This should never be called in a cooperative scheduler */
void timerInterrupt() {
    switch (sched_mode) {
        // this should never happen, but just in case
        case SchedulerMode::Cooperative:
            return;
        case SchedulerMode::Preemptive: {
            for (int i = 0; i < NUM_CORES && !ready_queue.empty(); i++) {
                if (cores[i]->running && !cores[i]->context_switching 
                && cores[i]->running->last_run != real_time) {
                    int pid = ready_queue.front();
                    ready_queue.pop();
                    ready_queue.push(cores[i]->running->PID);
                    contextSwitchOut(i);
                    contextSwitchIn(i, pid);
                }
            }
            break;    
        }
        case SchedulerMode::Hybrid: {
            for (int i = 0; i < NUM_CORES && !ready_queue.empty(); i++) {
                // makes sure process has ran for at least half a quantum to be considered to switch
                if (cores[i]->running && !cores[i]->context_switching 
                && cores[i]->running->last_run < real_time - 5) {
                    quantumBank(cores[i]->running, i);
                }
            }
            break;
        }
    }
    schedule();
}

/* Try to put waiting tasks into idle cores, if any */
void schedule() {
    for (int i = 0; i < NUM_CORES && !ready_queue.empty(); i++) {
        if (!cores[i]->running) {
            int pid = ready_queue.front();
            ready_queue.pop();
            contextSwitchIn(i, pid);
        }
    }
}

/* Puts process in ready queue of scheduler */
void processArrives(int pid) {
    ready_queue.push(pid);
    schedule();
}

/* Removes process forever, should be the process currently running */
bool processExits(int pid) {
    bool found = false;
    for (int core = 0; core < NUM_CORES; core++) {
        std::shared_ptr<Process> p = cores[core]->running;
        if (p && p->PID == pid) {
            p->completion = real_time;
            contextSwitchOut(core);
            found = true;
            break;
        }
    }
    if (found)
        schedule();
    return found;
}

