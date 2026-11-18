# Tests and workload input files

## Running the tests

`make test` builds the simulator (only if a source file changed), then runs
`tests/run_tests.sh` on every file in `tests/debug/` and `tests/inputs/` under each
mode and seed. Each run prints PASS or FAIL with a reason, and make exits nonzero if
anything fails. Every run's `RESULT` line is saved to `tests/results.csv` for the
results section of the paper.

Options (all optional):

- `make test MODES="0 1 2"`: modes to run (default `0 1`; hybrid is left out until it schedules)
- `make test SEEDS="0 1 2 3 4"`: seed offsets to run (default `0`)
- `make test TIMEOUT=120`: seconds before a run counts as hung (default 60)

Every run is checked for:

- **Clean exit:** status 0 within the timeout. Status 1 means processes were stranded; a timeout usually means a preemptive run where a process never finishes.
- **Lower bound:** no process completes before arrival + work + I/O time.
- **Cooperative switch count:** in mode 1, context switches equal processes plus I/O events, since nothing is ever preempted.
- **Debug expectations:** completions match the file's `# expect:` line (see Debug cases).

To add a test, drop a new input file into `tests/inputs/` (or `tests/debug/` with an
`# expect:` line). The script picks it up automatically.


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
`# expect: modes=0,1 sorted_completions=105`. `modes` lists which scheduler modes the
answer applies to, and `sorted_completions` is every process's completion time in
ascending order, so it doesn't depend on which PID finished first. If a commit changes one of these, either a
cost constant changed on purpose (update the comment) or something broke.