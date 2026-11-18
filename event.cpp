#include "event.h"
#include "scheduler.h"
#include "process.h"
#include "core.h"
#include "workload.h"
#include <stdio.h>
#include <climits>

int finished_proc = 0;

SchedulerMode sched_mode;

std::uint64_t real_time = 0;
std::priority_queue<Event, std::vector<Event>, Compare> event_queue;

/* Start the first timer event and run */
void startSimulation(SchedulerMode mode) {
    sched_mode = mode;
    initCores();
    if (mode == SchedulerMode::Preemptive) {
        Event e = {EventType::Timer, real_time = 10};
        event_queue.push(e);
    }

    // Avoid infinite loop
    if (process_list.empty()) {
        fprintf(stderr, "error: workload generated no processes\n");
        exit(2);
    }
    eventLoop();

    fprintf(stderr, "error: simulation ended with %d of %zu processes finished\n",
            finished_proc, process_list.size());
    printReport(false);
    exit(1);
}

void endSimulation() {
    printReport(true);
    printf("Finish!\n");
    exit(0);
}

std::uint64_t getRemainingQuantum() {
    if (sched_mode == SchedulerMode::Cooperative)
        return UINT_MAX;
    return TIME_QUANTUM - (real_time % TIME_QUANTUM);
}

/* Calculate whether process exit event should be added
 * Only push if less than time quantum, should never have exit events
 * between quantums/other events */
void addExitEvent(int core, int pid) {
    std::uint64_t rem = getRemainingQuantum();
    if (cores[core]->running->remaining <= rem) {
        Event e = {EventType::ProcessExit, 
            real_time + cores[core]->running->remaining, pid};
        event_queue.push(e);
    }
}

/* Calculate whether proccess IO start and end event 
 * should be added */
void addIOEvent(int pid) {
    std::shared_ptr<Process> p = getProcess(pid);
    if (!p->io.empty() && p->finished <= p->io.front().start_time &&
    p->finished + getRemainingQuantum() > p->io.front().start_time){
        int io_start = p->io.front().start_time - p->finished;
        Event e1 = {EventType::IOStart, real_time + io_start, pid};
        event_queue.push(e1);
    }
}

void calculateTimeSpent(std::shared_ptr<Process> p) {
    // calculate time spent
    if (p) {
        int runtime = real_time - p->last_run;
        p->remaining -= runtime;
        p->finished += runtime;
    }
}

bool staleExitEvent(int pid, std::uint64_t time) {
    // check which core is running this event and whether it's stale
    for (int i = 0; i < NUM_CORES; i++) {
        std::shared_ptr<Process> p = cores[i]->running;
        if (p && p->PID == pid && p->last_run + p->remaining == time
        && !cores[i]->context_switching) {
            return false;
        }
    }
    return true;
}

void Event::runEvent() {
    switch (this->type) {
        case EventType::Timer: {
            // check all cores to add exit events
            std::shared_ptr<Process> prev[NUM_CORES];
            for (int i = 0; i < NUM_CORES; i++) {
                prev[i] = cores[i]->running;
                if (!cores[i]->context_switching)
                    calculateTimeSpent(cores[i]->running);
            }
            timerInterrupt();
            // generate the next timer event
            Event e = {EventType::Timer, real_time + TIME_QUANTUM};
            event_queue.push(e);
            // only add exit/IO event if running same process as before
            for (int i = 0; i < NUM_CORES; i++) {
                if (prev[i] && (prev[i] == cores[i]->running) && !cores[i]->context_switching) {
                    cores[i]->running->last_run = real_time;
                    addIOEvent(cores[i]->running->PID);
                    addExitEvent(i, cores[i]->running->PID);
                }
            }
            break;
        }
        case EventType::ProcessArrival: {
            processArrives(this->pid);
            break;
        }
        case EventType::ProcessExit: {
            if (!staleExitEvent(this->pid, this->time) && processExits(this->pid)) {
                finished_proc++;
                if (static_cast<std::size_t>(finished_proc) == process_list.size())
                    endSimulation();
            }
            break;
        }
        // TODO
        case EventType::IOStart: {
            blockIO(this->pid);
            break;
        }
        case EventType::IOEnd: {
            processArrives(this->pid);
            break;
        }
        case EventType::ContextSwitchEnd: {
            contextSwitchFinish(this->pid);
            break;
        }
    }   
}

/* Event loop that runs timer events and process arrivals/exits
 * Jumps from one event time to the next for speed
 */
void eventLoop() {
    while(!event_queue.empty()) {
        Event next = event_queue.top();
        real_time = next.time;
        event_queue.pop();
        next.runEvent();    
    }
}
