Simulator to compare energy efficiency of preemptive and cooperative schedulers.

<br>How to run:
  - make test: runs all the test cases
  - make run [input file] [scheduler mode] [seed offset] [--dump]
      - seed and --dump is optional

<br>Things we are working on:
  - Jonah: cache
  - Caleb: idle c state energy heuristics
  - Chau: integrating energy and time latency of changing states

<br>Features of simulator:
  - Multi-core implementation
  - C states and P states
  - Core affinity/caches
  - I/O interface
  - Different scheduling modes
  - Energy calculations for context switching, running the scheduler, changing states, etc.