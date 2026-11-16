/* Process
 *
 * Contains processes' arrival time, remaining time, completion time, and PID
 * Work needed to be done by processes are effectively remaining time slices
 *
 * Created on 10/25/2026 for research purposes
 */

#pragma once
#include <vector>
#include <memory>
#include <cstdint>

/* IO items for when processes will need to block for IO */
struct IO {
    std::uint64_t start_time;
    std::uint64_t duration;
};

struct Process {
    std::vector<IO> io;
    std::uint64_t arrival;
    std::uint64_t completion;
    std::uint64_t last_run;
    signed int last_core;
    int finished;
    int remaining;
    int PID;
};

extern std::vector<std::shared_ptr<Process>> process_list;

std::shared_ptr<Process> getProcess(int pid);

void blockIO(int pid);
void yield(int pid);
void contextSwitchIn(int core, int pid);
void contextSwitchOut(int core);

