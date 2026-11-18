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

# Workload input files

Run: `./sim <input file> <mode> [seed offset] [--dump]` (mode 0 = preemptive, 1 = cooperative, 2 = hybrid).

An input file is one or more `process class:` stanzas, modeled on the CS 378 cloud
simulator's `task class:` format. `#` starts a comment. Keys are case-insensitive.

```
process class:
{
        Start time: 0            # first arrival lands exactly here
        End time: 60000          # no arrivals after this
        Inter arrival: 60        # mean gap, exponential (min 1 tick)
        Batch size: 1            # optional: processes per arrival (default 1)
        Runtime: [100, 300]      # uniform range, or a single number
        IO count: [3, 8]         # optional (default 0)
        IO gap: [10, 40]         # run time between I/Os; required if IO count > 0
        IO duration: [20, 50]    # blocked time per I/O; required if IO count > 0
        Process type: IO         # optional label, used only in TYPE report lines
        Seed: 520234
}
```

I/O that would start at or after the end of a process's runtime is dropped.
The seed offset from the command line is added to every class seed.

## Output

- `RESULT ...` one line: mode, status, end_time, context_switches, turnaround/wait/slowdown stats
- `TYPE type=... ...` one line per process type label
- `PROC pid=... ...` one line per process, only with `--dump`

`wait` = turnaround - work - I/O time (ready queue + context switches + affinity penalties).
`slowdown` = turnaround / (work + I/O time).

Exit status: 0 = all processes finished, 1 = event queue drained with processes stranded,
2 = bad input. Preemptive runs that hang never exit (the timer reschedules forever), so
always run with a timeout.

## Debug cases

Files in `debug/` have hand-traced answers in a comment, e.g.
`# expect: modes=0,1 sorted_completions=105`. If a commit changes one of these, either a
cost constant changed on purpose (update the comment) or something broke.