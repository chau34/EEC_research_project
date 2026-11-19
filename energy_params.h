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


/* Microseconds per simulator tick, sourced from lab machine config_hz, scaled down for practical use */
const uint64_t TICK_US = 1;

/*
 *  Power Calculations:
 *  P(k) = SHARED + k * P_ACTIVE, k = Number of Active Cores
 *  A sweep of the # of busy cores was run to find the average value
 *  P_active = Cov(k, P) / Var(k) = 7340 mW
 *  P_shared = mean(P) - P_active * mean(k) = 27541
 */

/* Power is read in milliwatts (mW) */

/* Active power of one core running as normal */
const uint64_t P_ACTIVE_MW = 8310; // 7340 (Power added per core) + C1_IDLE_POWER_MW

/* Non-core power used */
const uint64_t SHARED_ACTIVE_MW = 27541; // Interecpt of active - 16 * C1_IDLE_POWER_MW

/* Package power of cores in idle, k = 0 */
const uint64_t SHARED_IDLE_MW = 27459; // Shared Idle - 16 * C1_IDLE_POWER_MW

/* 
 *  C1 idle state is technically C1E
 *  idle power sourced from academic paper
 */
const uint64_t C1_IDLE_POWER_MW = 880;
const uint64_t C1_EXIT_US = 10;
const uint64_t C1_TARGET_RESIDENCY_US = 20;

/* 
 *  C3 derived from C1 and C6
 *  US is derived logarithmically due to the exponential different in times
 *  490 is the midpoint between C1 power and C6 power
 *  C3 was not measured in academic paper
 */
const uint64_t C3_IDLE_POWER_MW = 490;
const uint64_t C3_EXIT_US = 30;
const uint64_t C3_TARGET_RESIDENCY_US = 74;

/* 
 *  C6 Found from intels table detailing heuristics 
 *  idle power sourced from academic paper
 */
const uint64_t C6_IDLE_POWER_MW = 100;
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