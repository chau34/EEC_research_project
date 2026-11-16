#include "event.h"
#include "scheduler.h"
#include "process.h"
#include "core.h"
#include <random>
#include <stdio.h>

#define PROCESSES 1000

int finished_proc = 0;

std::uint64_t real_time = 0;
std::priority_queue<Event, std::vector<Event>, Compare> event_queue;

/* Start the first timer event and run */
void startSimulation() {
    initCores();
    Event e = {EventType::Timer, real_time = 10};
    event_queue.push(e);
    eventLoop();
}

void endSimulation() {
    printf("Finish!\n");
    exit(0);
}

void generateWorkload(int seed) {
    std::mt19937 generator(seed);

    // TODO: add other types of workload distribution
    std::uniform_int_distribution<int> gap_dist (1, 100);
    std::uniform_int_distribution<int> work_dist (100, 1000);
    std::uniform_int_distribution<int> io_num_dist (1, 5);
    std::uniform_int_distribution<int> io_gap_dist (5, 15);
    std::uniform_int_distribution<int> io_duration_dist (20, 50);

    // TODO: change standard number of processes? is 1000 processes too much?
    std::uint64_t last_time = 0;
    for (int i = 0; i < PROCESSES; i++) {
        std::uint64_t arrival = last_time + gap_dist(generator);
        last_time = arrival;
        int workload = work_dist(generator);
        // generate IO times and durations
        int io_num = io_num_dist(generator);
        std::vector<IO> io;
        int io_start = 0;
        for (int i = 0; i < io_num; i++) {
            io_start += io_gap_dist(generator);
            int io_duration = io_duration_dist(generator);
            io.push_back(IO{io_start, io_duration});
        }
        std::shared_ptr<Process> p = std::make_shared<Process>(
            Process{io, arrival, 0, 0, -1, 0, workload, i});
        process_list.push_back(p);
        Event e = {EventType::ProcessArrival, arrival, i};
        event_queue.push(e);
    }
}

int getRemainingQuantum() {
    return TIME_QUANTUM - (real_time % TIME_QUANTUM);
}

/* Calculate whether process exit event should be added
 * Only push if less than time quantum, should never have exit events
 * between quantums/other events */
void addExitEvent(int core, int pid) {
    int rem = getRemainingQuantum();
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

bool staleExitEvent(int pid, int time) {
    // check which core is running this event and whether it's stale
    bool stale = true;
    for (int i = 0; i < NUM_CORES; i++) {
        std::shared_ptr<Process> p = cores[i]->running;
        if (p && p->PID == pid && p->last_run + p->remaining == time) {
            stale = false;
            break;
        }
    }
    return stale;
}

void cache_update() {
    for (int i = 0; i < NUM_CORES; i++) {
        if (real_time - cores[i]->last_cache_flush >= FLUSH_QUANTUM) {
            cores[i]->last_cache_flush = real_time;
            // TODO: energy used by flushing?
        }
    }
}

void Event::runEvent() {
    switch (this->type) {
        case EventType::Timer: {
            // check all cores to add exit events
            std::shared_ptr<Process> prev[NUM_CORES];
            for (int i = 0; i < NUM_CORES; i++) {
                prev[i] = cores[i]->running;
                calculateTimeSpent(cores[i]->running);
            }
            timerInterrupt();
            // generate the next timer event
            Event e = {EventType::Timer, real_time + TIME_QUANTUM};
            event_queue.push(e);
            // only add exit/IO event if running same process as before
            for (int i = 0; i < NUM_CORES; i++) {
                if (prev[i] && (prev[i] == cores[i]->running)) {
                    cores[i]->running->last_run = real_time;
                    addIOEvent(cores[i]->running->PID);
                    addExitEvent(i, cores[i]->running->PID);
                }
            }
            // flush caches to simulate eviction/full cache
            cache_update();
            break;
        }
        case EventType::ProcessArrival: {
            processArrives(this->pid);
            break;
        }
        case EventType::ProcessExit: {
            if (!staleExitEvent(this->pid, this->time) && processExits(this->pid)) {
                finished_proc++;
                if (finished_proc == PROCESSES)
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

