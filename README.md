Simulator to compare energy efficiency of preemptive and cooperative schedulers.
Things to work on:
  - Multi-core implementation (in progress - Chau)
  - Energy calculations for context switching and running the scheduler
  - Separation between preemptive and cooperative scheduling algorithms (currently running preemptive Round Robin)
  - Context switching timing delay (currently takes no time to context switch)
  - Fancy stuff to emulate real systems: 
    - yielding
    - core affinity/caches
    - thermal design power
    - C states and P states (in progress - Chau)
  - I/O interface and energy consumption of storage devices
  - Better process implementation for various types of processes in a system (eg. CPU-bound or I/O bound processes, different kinds of workload)
