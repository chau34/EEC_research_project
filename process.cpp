#include "process.h"
#include "event.h"
#include "scheduler.h"
#include "core.h"

std::vector<std::shared_ptr<Process>> process_list;

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

void yield() {
    // TODO: calculate remaining again because we yield before finishing time quantum
            // if not done so already
    
}
    

void contextSwitchIn(int pid) {
    current_process = getProcess(pid);
    // generate exit event if current process will finish before time quantum
    if (current_process)
        addExitEvent(current_process->PID);
    // TODO: energy count
}

void contextSwitchOut() {
    current_process = NULL;
    // TODO: energy count
}


std::shared_ptr<Process> getProcess(int pid) {
    return process_list.at(pid);
}