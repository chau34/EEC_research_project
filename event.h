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

enum class EventType {
    Timer,
    ProcessArrival,
    ProcessExit
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
        return time1 > time2;
    }
};

void startSimulation();

void generateWorkload(int seed);

void addExitEvent(int pid);

extern std::uint64_t real_time;
extern std::priority_queue<Event, std::vector<Event>, Compare> event_queue;

void eventLoop();