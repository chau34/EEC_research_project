/* Event
 *
 * Effectively the event loop, generate and run events 
 * Create timer interrupts, queue of process arrivals/exits
 * Using seeds to ensure fair benchmarks between the scheduling policies
 * 
 * Created on 10/25/2026 for research purposes
 */

#pragma once
#include <queue>
#include <cstdint>
#include <vector>
#include <memory>
#include "process.h"

extern std::uint64_t total_energy_nj;

enum class SchedulerMode {
    Preemptive,
    Cooperative,
    Hybrid
};

extern SchedulerMode sched_mode;

/* DO NOT change the order of the event types */
enum class EventType {
    ProcessExit,
    Timer,
    ProcessArrival,
    IOStart,
    IOEnd,
    ContextSwitchEnd,
    TimerEnd,
    ProcessYield
};

struct Event {
    EventType type;
    std::uint64_t time;
    int pid;
    void runEvent();
    std::uint64_t getTime();
};

class Compare {
public:
    bool operator() (Event e1, Event e2) {
        std::uint64_t time1 = e1.time;
        std::uint64_t time2 = e2.time;
        if (time1 == time2) {
            // tie breaker: exit, timer, arrival
            return static_cast<int>(e1.type) > static_cast<int>(e2.type);
        }
        return time1 > time2;
    }
};

void startSimulation(SchedulerMode mode);

std::uint64_t getRemainingQuantum();

void addYieldEvent(int pid, int duration);
void addIOEvent(int pid);
void addExitEvent(int core, int pid);

void calculateTimeSpent(std::shared_ptr<Process> p);

extern std::uint64_t real_time;
extern std::priority_queue<Event, std::vector<Event>, Compare> event_queue;

void eventLoop();
