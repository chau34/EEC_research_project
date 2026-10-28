#include "scheduler.h"
#include "process.h"
#include "event.h"

std::queue<int> ready_queue;
std::shared_ptr<Process> current_process;

/* This will never be called in a cooperative scheduler
 * as TIME_QUANTUM will be infinite */
void timerInterrupt() {
    schedule();
}

void schedule() {
    if (!ready_queue.empty()) {
        int pid = ready_queue.front();
        ready_queue.pop();
        if (current_process)
            ready_queue.push(current_process->PID);
        contextSwitchOut();
        contextSwitchIn(pid);    
    }
}

/* Puts process in ready queue of scheduler */
void processArrives(int pid) {
    ready_queue.push(pid);
    if (!current_process)
        schedule();
}

/* Removes process forever, should be the process currently running */
void processExits(int pid) {
    std::shared_ptr<Process> p = getProcess(pid);
    p->completion = real_time;
    current_process = NULL;
    schedule();
}

