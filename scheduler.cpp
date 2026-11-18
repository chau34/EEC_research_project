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
            if (!ready_queue.empty()) {
                int pid = ready_queue.front();
                ready_queue.pop();
                if (cores[0]->running)
                    ready_queue.push(cores[0]->running->PID);
                contextSwitchOut(0);
                contextSwitchIn(0, pid);    
            }
            return;
        }
        case SchedulerMode::Cooperative: {
            // TODO: loop through all cores and assign process, if any
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
    if (!cores[0]->running)
        schedule();
}

/* Removes process forever, should be the process currently running */
bool processExits(int pid) {
    bool found = false;
    for (int core = 0; core < NUM_CORES; core++) {
        std::shared_ptr<Process> p = cores[core]->running;
        if (p && p->PID == pid) {
            p->completion = real_time;
            cores[core]->running = NULL;
            found = true;
            break;
        }
    }
    // printf("HERE %d\n", found);
    if (found)
        schedule();
    return found;
}

