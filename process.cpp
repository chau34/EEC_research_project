#include "process.h"
#include "event.h"
#include "scheduler.h"
#include "core.h"
#include "energy_params.h"

#define CORE_AFFINITY_PENALTY 3

std::vector<std::shared_ptr<Process>> process_list;
std::uint64_t context_switches = 0;

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

/* Flush the cache at C3 */
void C3CacheFlush(std::shared_ptr<Core> core) {
    std::uint64_t idle = real_time - core->last_idle;
    if (idle >= C3_TARGET_RESIDENCY_US) {
        std::uint64_t new_flush = real_time - C3_TARGET_RESIDENCY_US;
        if (core->last_cache_flush < new_flush)
            core->last_cache_flush = new_flush;
    }
}

std::uint64_t stateWakeLatency(std::shared_ptr<Core> core) {
    std::uint64_t idle = real_time - core->last_idle;
    if (idle >= C6_TARGET_RESIDENCY_US)
        return C6_EXIT_US;
    else if (idle >= C3_TARGET_RESIDENCY_US)
        return C3_EXIT_US;
    else if (idle >= C1_TARGET_RESIDENCY_US)
        return C1_EXIT_US;
    return 0;
}

void contextSwitchIn(int core_id, int pid) {
    std::shared_ptr<Core> core = cores[core_id];
    C3CacheFlush(core);
    std::shared_ptr<Process> process = getProcess(pid);
    core->context_switching = true;
    core->running = process;
    context_switches++;
    event_queue.push({EventType::ContextSwitchEnd, 
        real_time + stateWakeLatency(core) + CONTEXT_SWITCH_US, pid});
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
            return;
        }
    } 
}

void contextSwitchOut(int core_id) {
    cores[core_id]->running = NULL;
    // this is safety net for failed context switch
    cores[core_id]->context_switching = false;
    cores[core_id]->last_idle = real_time;
}


std::shared_ptr<Process> getProcess(int pid) {
    return process_list.at(pid);
}
