Simulator to compare energy efficiency of preemptive and cooperative schedulers.

<br>Things we are working on:
  - Jonah: energy calculations
  - Caleb: testing interface
  - Chau: time latency of context switch

<br>Things implemented but can be improved upon: 
  - Multi-core implementation
  - C states and P states (need time latency and finish events...)
  - core affinity/caches
  - I/O interface
  - Separation between scheduling mode (can now have args with make run)

<br>Things to start:
  - Energy calculations for context switching, running the scheduler, changing states, etc.
  - Context switching timing delay (currently takes no time to context switch)
  - Test cases:
      - add make test to Makefile
      - better process implementation for various types of processes in a system (eg. CPU-bound or I/O bound processes, different kinds of workload)
  - Fancy: thermal design power?
  - Energy consumption of storage devices
