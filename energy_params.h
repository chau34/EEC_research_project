#pragma once
#include <cstdint>
#include <cstdio>

/*   
 *  Lab Machine Specs
 *  CPU: Gold 6226R, Cascade Lake-SP
 *  2×16 cores with SMT
 *  kernel 7.0.0-34-generic, microcode 0x5003901
 *  intel_cpufreq/schedutil, turbo on
 */


/* Microseconds per simulator tick, sourced from lab machine config_hz */
const uint64_t TICK_US = 1000;

/*
 *  Power Calculations:
 *  P(k) = SHARED + k * P_ACTIVE, k = Number of Active Cores
 *  A sweep of the # of busy cores was run to find the average value
 *  P_active = Cov(k, P) / Var(k) = 87584 / 24 = 3649 mW
 *  P_shared = mean(P) - P_active * mean(k) = 68654 - (3649 * 8) = 39459
 */

/* Power is read in milliwatts (mW) */

/* Active power of one core running as normal */
const uint64_t P_ACTIVE_MW = 3649;

/* Non-core power used */
const uint64_t SHARED_ACTIVE_MW = 39459;

/* Package power of cores in idle, k = 0 */
const uint64_t SHARED_IDLE_MW = 41403;

/* C1 idle state is technically C1E */
const uint64_t C1_IDLE_POWER_MW = 0;
const uint64_t C1_EXIT_US = 10;
const uint64_t C1_TARGET_RESIDENCY_US = 20;

/* TODO: Caleb please put real numbers here */
const uint64_t C3_IDLE_POWER_MW = 0;
const uint64_t C3_EXIT_US = 20;
const uint64_t C3_TARGET_RESIDENCY_US = 50;

/* C6 Found from intels table detailing heuristics */
const uint64_t C6_IDLE_POWER_MW = 0;
const uint64_t C6_EXIT_US = 92;
const uint64_t C6_TARGET_RESIDENCY_US = 276;

/* Measured in timer_interrupt_calc.c by measuring the duration of an interrupt */
const uint64_t TIMER_INTERRUPT_US = 3;

/* Captured in CONTEXT_SWITCH_US */
const uint64_t SCHEDULER_INVOCATION_US = 0;

/* Calculated in context_switch_calc.c on the lab machine */
/* Average value: 1us per switch */
const uint64_t CONTEXT_SWITCH_US = 1;

/* Measured in cache_calc.c after timing forced evictions */

const uint64_t REFILL_MIGRATE_US = 50;
const uint64_t REFILL_POSTFLUSH_US = 112;
const uint64_t REFILL_POSTFLUSH_LINES = 4096;