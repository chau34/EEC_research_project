#include "scheduler.h"
#include "process.h"
#include "core.h"
#include <iostream>

std::queue<int> ready_queue;

/* This will never be called in a cooperative scheduler
 * as TIME_QUANTUM will be infinite */
void timerInterrupt() {
    schedule();
}

void schedule() {
    switch(sched_mode) {
        case SchedulerMode::Preemptive: {
            for (int i = 0; i < NUM_CORES && !ready_queue.empty(); i++) {
                int pid = ready_queue.front();
                ready_queue.pop();
                if (cores[i]->running)
                    ready_queue.push(cores[i]->running->PID);
                contextSwitchOut(i);
                contextSwitchIn(i, pid);
            }
            return;
        }
        case SchedulerMode::Cooperative: {
            for (int i = 0; i < NUM_CORES && !ready_queue.empty(); i++) {
                if (!cores[i]->running) {
                    int pid = ready_queue.front();
                    ready_queue.pop();
                    contextSwitchIn(i, pid);
                }
            }
            return;
        }
        case SchedulerMode::Hybrid: {
            // TODO
            return;
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

