#include "process.h"
#include "event.h"
#include "scheduler.h"
#include "core.h"

#define CORE_AFFINITY_PENALTY 3

std::vector<std::shared_ptr<Process>> process_list;

// /* Used for yield in case exit event is scheduled, 
//  * but this code is wrong since it assumes top event is process_exit.
//  * Either way, won't need this code if time remaining takes into consideration
//  * yield event when context switch in
//  */
// void removeExitEvent() {
//     Event e = event_queue.top();
//     if (e.type == EventType::ProcessExit)
//         event_queue.pop();
// }

/* Used for cooperative when IO event occurs */
void yield() {
    // TODO: calculate time spent of current process because yield before finishing time quantum
    
}

void contextSwitchIn(int core_id, int pid) {
    std::shared_ptr<Core> core = cores[core_id];
    std::shared_ptr<Process> process = getProcess(pid);

    // core affinity penalty
    if ((process->last_core != core_id)
    || (core->last_cache_flush >= process->last_run))
        process->remaining += CORE_AFFINITY_PENALTY;

    core->running = process;
    core->running->last_run = real_time;
    // TODO: generate IO event if there is one within time quantum
    // generate exit event if current process will finish before time quantum
    addExitEvent(core_id, core->running->PID);
    // TODO: energy count
    
}

void contextSwitchOut(int core) {
    cores[core]->running = NULL;
    // TODO: energy count
}


std::shared_ptr<Process> getProcess(int pid) {
    return process_list.at(pid);
}