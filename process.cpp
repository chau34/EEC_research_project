#include "process.h"
#include "event.h"
#include "scheduler.h"
#include "core.h"

#define CORE_AFFINITY_PENALTY 3
#define CONTEXT_SWITCH_COST 1

std::vector<std::shared_ptr<Process>> process_list;
std::uint64_t context_switches = 0;

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

/* Used when IO event occurs */
void blockIO(int pid) {
    std::shared_ptr<Process> p = getProcess(pid);
    // stale IO event
    if (p->last_core < 0 || cores[p->last_core]->running != p
    || cores[p->last_core]->context_switching)
        return;
    calculateTimeSpent(p);
    Event e2 = {EventType::IOEnd, real_time + p->io.front().duration, pid};
    event_queue.push(e2);
    p->io.erase(p->io.begin());
    contextSwitchOut(p->last_core);
    schedule();
}

void yield(int pid) {
    // TODO: calculate time spent of current process because yield before finishing time quantum
    
}

void contextSwitchIn(int core_id, int pid) {
    std::shared_ptr<Core> core = cores[core_id];
    std::shared_ptr<Process> process = getProcess(pid);
    core->context_switching = true;
    core->running = process;
    context_switches++;
    event_queue.push({EventType::ContextSwitchEnd, real_time + CONTEXT_SWITCH_COST, pid});
}

void contextSwitchFinish(int pid) {
    std::shared_ptr<Process> process = getProcess(pid);

    // find the core that was context switching in this process
    for (int i = 0; i < NUM_CORES; i++) {
        std::shared_ptr<Core> core = cores[i];
        if (core->running && core->running->PID == pid && core->context_switching) {
            // core affinity penalty
            if ((process->last_core != core->core_num) || core->last_pid != pid
            || (core->last_cache_flush >= process->last_run))
                process->remaining += CORE_AFFINITY_PENALTY;

            core->context_switching = false;
            core->running->last_core = core->core_num;
            core->running->last_run = real_time;
            core->last_pid = pid;

            // generate IO event if there is one within time quantum
            addIOEvent(pid);
            // generate exit event if current process will finish before time quantum
            addExitEvent(core->core_num, pid);
            // TODO: energy count
            
            return;
        }
    } 
}

void contextSwitchOut(int core_id) {
    cores[core_id]->running = NULL;
    // this is safety net for failed context switch
    cores[core_id]->context_switching = false;
    // TODO: energy count
}


std::shared_ptr<Process> getProcess(int pid) {
    return process_list.at(pid);
}
