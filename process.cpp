#include "process.h"
#include "event.h"
#include "scheduler.h"
#include "core.h"
#include "energy_params.h"

#define CORE_AFFINITY_PENALTY 3
#define CACHE_HOT_PENALTY 50
#define CACHE_HOT_MAX 100
#define CACHE_HOT_RATIO 0

std::vector<std::shared_ptr<Process>> process_list;
std::uint64_t context_switches = 0;

/* Used when IO event occurs */
void blockIO(int pid) {
    std::shared_ptr<Process> p = getProcess(pid);
    // stale IO event
    if (p->last_core < 0 || cores[p->last_core]->running != p
    || cores[p->last_core]->context_switching || cores[p->last_core]->timer_interrupted)
        return;
    calculateTimeSpent(p);
    Event e2 = {EventType::IOEnd, real_time + p->io.front().duration, pid};
    event_queue.push(e2);
    // add duration of cache hot after I/O
    // TODO: take into account cache decay if page coloring is not implemented
    p->cache_hot_duration = CACHE_HOT_RATIO * p->io.front().duration;
    // cap max for duration because cache has max capacity too
    if (p->cache_hot_duration > CACHE_HOT_MAX)
        p->cache_hot_duration = CACHE_HOT_MAX;
    p->context_switched_from_io = true;
    // caps the cache hot duration to the next I/O
    p->cache_hot_until = 0;
    p->context_switched_hot = false;
    p->io.erase(p->io.begin());
    contextSwitchOut(p->last_core);
    schedule();
}

/* Flush the cache at C3 */
void C3CacheFlush(std::shared_ptr<Core> core) {
    std::uint64_t idle = real_time - core->last_idle;
    if (idle >= C3_TARGET_RESIDENCY_US) {
        std::uint64_t new_flush = core->last_idle + C3_TARGET_RESIDENCY_US;
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

// TODO: realistic core affinity penalty
std::uint64_t cacheLatency(std::shared_ptr<Core> core, std::shared_ptr<Process> process) {
    if (process->context_switched_hot) {
        process->context_switched_hot = false;
        return CACHE_HOT_PENALTY;
    }
    if ((process->last_core != core->core_num) || core->last_pid != process->PID
    || (core->last_cache_flush >= process->last_run))
        return CORE_AFFINITY_PENALTY;
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
        real_time + stateWakeLatency(core) + CONTEXT_SWITCH_US + cacheLatency(core, process), pid});
}

void contextSwitchFinish(int pid) {
    std::shared_ptr<Process> process = getProcess(pid);

    // find the core that was context switching in this process
    for (int i = 0; i < NUM_CORES; i++) {
        std::shared_ptr<Core> core = cores[i];
        if (core->running && core->running->PID == pid && core->context_switching) {
            core->context_switching = false;
            core->running->last_core = core->core_num;
            core->running->last_run = real_time;
            core->last_pid = pid;

            // set up cache hotness if comes from IO
            // TODO: cache decay or cache eviction smth
            if (process->context_switched_from_io) {
                process->cache_hot_until = real_time + process->cache_hot_duration;
                process->context_switched_from_io = false;
            }

            // generate IO event if there is one within time quantum
            addIOEvent(pid);
            // generate exit event if current process will finish before time quantum
            addExitEvent(core->core_num, pid);
            return;
        }
    } 
}

void contextSwitchOut(int core_id) {
    std::shared_ptr<Core> core = cores[core_id];
    // cache was hot, will cause extra latency when context switch in
    if (real_time < core->running->cache_hot_until) {
        core->running->context_switched_hot = true;
        core->running->cache_hot_until = 0;
    }
        
    core->running = NULL;
    // this is safety net for failed context switch
    core->context_switching = false;
    core->last_idle = real_time;
}


std::shared_ptr<Process> getProcess(int pid) {
    return process_list.at(pid);
}
