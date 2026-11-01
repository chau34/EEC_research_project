#include "event.h"
#include "scheduler.h"
#include "process.h"
#include <random>
#include <stdio.h>

#define PROCESSES 1000

int finished_proc = 0;

std::uint64_t real_time = 0;
std::priority_queue<Event, std::vector<Event>, Compare> event_queue;

/* Start the first timer event and run */
void startSimulation() {
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
    std::uniform_int_distribution<int> work_dist (5, 50);

    // TODO: change standard number of processes? is 1000 processes too much?
    std::uint64_t last_time = 0;
    for (int i = 0; i < PROCESSES; i++) {
        std::uint64_t arrival = last_time + gap_dist(generator);
        last_time = arrival;
        int workload = work_dist(generator);
        std::shared_ptr<Process> p = std::make_shared<Process>(Process{arrival, 0, 0, workload, i});
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
 * between quantums/other events
 */
void addExitEvent(int core, int pid) {
    int rem = getRemainingQuantum();
    if (current_process[core]->remaining <= rem) {
        Event e = {EventType::ProcessExit, 
            real_time + current_process[core]->remaining, pid};
        event_queue.push(e);
    }
}

void calculateTimeSpent(int core) {
    // calculate time spent
    std::shared_ptr<Process> p = current_process[core];
    if (p)
        p->remaining -= real_time - p->last_run;
}

bool staleExitEvent(int pid, int time) {
    // check which core is running this event and whether it's stale
    bool stale = true;
    for (int i = 0; i < NUM_CORES; i++) {
        std::shared_ptr<Process> p = current_process[i];
        if (p && p->PID == pid && p->last_run + p->remaining == time) {
            stale = false;
            break;
        }
    }
    return stale;
}

void Event::runEvent() {
    switch (this->type) {
        case EventType::Timer: {
            // check all cores to add exit events
            std::shared_ptr<Process> prev[NUM_CORES];
            for (int i = 0; i < NUM_CORES; i++) {
                prev[i] = current_process[i];
                calculateTimeSpent(i);
            }
            timerInterrupt();
            // generate the next timer event
            Event e = {EventType::Timer, real_time + TIME_QUANTUM};
            event_queue.push(e);
            // only add exit event if running same process as before
            for (int i = 0; i < NUM_CORES; i++) {
                if (prev[i] && (prev[i] == current_process[i])) {
                    current_process[i]->last_run = real_time;
                    addExitEvent(i, current_process[i]->PID);
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
                if (finished_proc == PROCESSES)
                    endSimulation();
            }
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

