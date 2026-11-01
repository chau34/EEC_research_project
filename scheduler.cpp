#include "scheduler.h"
#include "process.h"
#include "event.h"
#include <iostream>

std::queue<int> ready_queue;
std::array<std::shared_ptr<Process>, NUM_CORES> current_process;

/* This will never be called in a cooperative scheduler
 * as TIME_QUANTUM will be infinite */
void timerInterrupt() {
    schedule();
}

void schedule() {
    if (!ready_queue.empty()) {
        int pid = ready_queue.front();
        ready_queue.pop();
        if (current_process.at(0))
            ready_queue.push(current_process[0]->PID);
        contextSwitchOut(0);
        contextSwitchIn(0, pid);    
    }
}

/* Puts process in ready queue of scheduler */
void processArrives(int pid) {
    ready_queue.push(pid);
    if (!current_process.at(0))
        schedule();
}

/* Removes process forever, should be the process currently running */
bool processExits(int pid) {
    bool found = false;
    for (int core = 0; core < NUM_CORES; core++) {
        std::shared_ptr<Process> p = current_process[core];
        if (p && p->PID == pid) {
            p->completion = real_time;
            current_process[core] = NULL;
            found = true;
            break;
        }
    }
    // printf("HERE %d\n", found);
    if (found)
        schedule();
    return found;
}

