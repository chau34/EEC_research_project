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
        std::shared_ptr<Process> p = std::make_shared<Process>(Process{arrival, 0, workload, i});
        process_list.push_back(p);
        Event e = {EventType::ProcessArrival, arrival, i};
        event_queue.push(e);
    }
}

int getRemainingQuantum() {
    return real_time - (real_time % TIME_QUANTUM);
}

/* Calculate whether process exit event should be added
 * Only push if less than time quantum, should never have exit events
 * between quantums/other events
 */
void addExitEvent(int pid) {
    // TODO: possibly change calculation if yield in future
    int rem = getRemainingQuantum();
    if (current_process->remaining <= rem) {
        Event e = {EventType::ProcessExit, 
            real_time + current_process->remaining, pid};
        event_queue.push(e);
    } else {
        current_process->remaining -= rem;
    }
}

void Event::runEvent() {
    switch (this->type) {
        case EventType::Timer: {
            std::shared_ptr<Process> prev = current_process;
            timerInterrupt();
            // generate the next timer event
            Event e = {EventType::Timer, real_time + TIME_QUANTUM};
            event_queue.push(e);
            // only add exit event if running same process as before
            // else contextSwitchIn() should handle exit event of new process
            if (prev && (prev == current_process))
                addExitEvent(current_process->PID);
            break;
        }
        case EventType::ProcessArrival: {
            processArrives(this->pid);
            break;
        }
        case EventType::ProcessExit: {
            processExits(this->pid);
            finished_proc++;
            if (finished_proc == PROCESSES)
                endSimulation();
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

