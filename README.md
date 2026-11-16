Simulator to compare energy efficiency of preemptive and cooperative schedulers.
Things implemented but can be improved upon: 
  - Multi-core implementation
  - C states and P states
  - core affinity/caches
  - I/O interface
Things to start:
  - Energy calculations for context switching, running the scheduler, changing states, etc.
  - Separation between preemptive and cooperative scheduling algorithms (currently running preemptive Round Robin)
  - Context switching timing delay (currently takes no time to context switch)
  - Test cases (and add make test to Makefile)
  - Fancy stuff to emulate real systems: 
    - yielding
    - thermal design power
  - Energy consumption of storage devices
  - Better process implementation for various types of processes in a system (eg. CPU-bound or I/O bound processes, different kinds of workload)
