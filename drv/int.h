/**
 * @file int.h
 * @brief Interrupt controller driver — inline accessors for the AVR-Dx CPUINT.
 *
 * Provides a thin, zero-overhead set of `static inline` functions that
 * manipulate the registers of the CPU Interrupt Controller (CPUINT). The
 * AVR-Dx has a single interrupt controller, so these functions operate directly
 * on the global `CPUINT` register set rather than on a caller-supplied pointer.
 *
 * The driver covers interrupt-priority scheduling (round-robin enable, the
 * round-robin/low-priority vector, and the single level-1 high-priority
 * vector), the vector-table configuration bits (vectors-in-boot and the compact
 * vector table), and the read-only execution-status flags.
 *
 * @note This driver configures the interrupt *controller* peripheral. The
 *       global interrupt enable (the CPU SREG I-bit, `sei`/`cli`) is a core
 *       function, not a CPUINT register, and is exposed separately as
 *       `ENABLE_INTERRUPTS()` / `DISABLE_INTERRUPTS()` in drv/cpu.h.
 *
 * @note The IVSEL and CVT bits in CTRLA are protected by the Configuration
 *       Change Protection (CCP) mechanism and require a timed unlock sequence to
 *       write. `intVectorsInBoot()` and `intCompactVectorTableEnable()` perform
 *       this via avr-libc's `ccp_write_io()`. These vector-table settings should
 *       be configured during start-up before global interrupts are enabled.
 *
 * ### Interrupt safety
 *
 * The non-protected accessors are plain 8-bit register pokes with no internal
 * interrupt masking; the read-modify-write functions (e.g.
 * `intRoundRobinEnable()`) can lose a concurrent ISR update to the same
 * register. In practice the controller is configured once at start-up, before
 * `sei()`, so no masking is needed; if you reconfigure it while interrupts are
 * live, serialize the access with an `ATOMIC_BLOCK` (see `<util/atomic.h>`).
 * All CPUINT registers are 8-bit, so there is no byte-pair (`TEMP`) hazard.
 *
 * The bit-mask (`_bm`) and group-code (`_gc`) symbols referenced here are
 * supplied by `<avr/io.h>` for the selected device. Interrupt vector numbers
 * are provided by the `<peripheral>_<source>_vect_num` macros in `<avr/io.h>`.
 *
 * Created: 6/27/2026
 * Author: john anderson
 *
 * Copyright (C) 2021 by John Anderson <racerxr650r@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef INT_H_
#define INT_H_

/** @addtogroup int_driver
 * @{
 */

// Includes -------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/cpufunc.h>

// Constants ------------------------------------------------------------------
/**
 * @brief Disable the level-1 (high-priority) vector.
 *
 * Pass to `intSetHighPriorityVector()` to clear the high-priority assignment so
 * that all enabled interrupts use level-0 priority.
 */
#define INT_NO_HIGH_PRIORITY	0

// Data Types -----------------------------------------------------------------
/**
 * @brief CPUINT execution-status flag selector.
 *
 * Bit mask identifying the read-only execution-status flags in the STATUS
 * register. Values may be OR'd together when testing the result of
 * `intGetStatus()`. The bit positions match the STATUS register.
 */
typedef enum
{
	INT_STATUS_LVL0EX = CPUINT_LVL0EX_bm,	///< A level-0 interrupt is executing
	INT_STATUS_LVL1EX = CPUINT_LVL1EX_bm,	///< A level-1 interrupt is executing
	INT_STATUS_NMIEX  = CPUINT_NMIEX_bm,	///< A non-maskable interrupt is executing
	INT_STATUS_ALL    = CPUINT_LVL0EX_bm | CPUINT_LVL1EX_bm |
	                    CPUINT_NMIEX_bm	///< All status flags
} intStatus_t;

// Inline Functions -----------------------------------------------------------
// Priority scheduling --------------------------------------------------------
/**
 * @brief Enable or disable round-robin scheduling of level-0 interrupts.
 *
 * Controls the LVL0RR bit in CTRLA. When enabled, the priority of level-0
 * interrupts rotates after each acknowledged interrupt (the last-served vector
 * becomes the lowest priority), preventing a single high-numbered vector from
 * starving the others. When disabled, level-0 vectors use their fixed
 * vector-number priority. Other CTRLA bits are preserved.
 *
 * @param enable true to enable round-robin scheduling, false for fixed priority.
 */
static inline void intRoundRobinEnable(bool enable)
{
	if(enable)
		CPUINT.CTRLA |= CPUINT_LVL0RR_bm;
	else
		CPUINT.CTRLA &= ~CPUINT_LVL0RR_bm;
}

/**
 * @brief Set the round-robin / lowest-priority level-0 vector.
 *
 * Writes the LVL0PRI register with an interrupt vector number. The vector with
 * the *next* number becomes the highest-priority level-0 interrupt and priority
 * decreases from there, wrapping around — so this selects the vector assigned
 * the lowest priority. When round-robin scheduling is enabled the hardware
 * updates this register automatically after each interrupt; writing 0 gives all
 * level-0 vectors their default fixed priority.
 *
 * @param vectorNumber Interrupt vector number (`<src>_vect_num`).
 */
static inline void intSetRoundRobinPriority(uint8_t vectorNumber)
{
	CPUINT.LVL0PRI = vectorNumber;
}

/**
 * @brief Read the current round-robin / lowest-priority level-0 vector.
 *
 * @return The vector number currently held in LVL0PRI.
 */
static inline uint8_t intGetRoundRobinPriority(void)
{
	return CPUINT.LVL0PRI;
}

/**
 * @brief Assign one interrupt vector to level-1 (high) priority.
 *
 * Writes the LVL1VEC register with the vector number to promote to level 1.
 * Exactly one vector may be level 1; a level-1 interrupt can preempt a level-0
 * interrupt. Pass `INT_NO_HIGH_PRIORITY` to disable the high-priority
 * assignment.
 *
 * @param vectorNumber Interrupt vector number (`<src>_vect_num`), or
 *                     `INT_NO_HIGH_PRIORITY`.
 */
static inline void intSetHighPriorityVector(uint8_t vectorNumber)
{
	CPUINT.LVL1VEC = vectorNumber;
}

/**
 * @brief Read the vector currently assigned to level-1 (high) priority.
 *
 * @return The vector number in LVL1VEC, or `INT_NO_HIGH_PRIORITY` if none.
 */
static inline uint8_t intGetHighPriorityVector(void)
{
	return CPUINT.LVL1VEC;
}

// Vector-table configuration (CCP protected) ---------------------------------
/**
 * @brief Select whether the interrupt vectors reside in the boot section.
 *
 * Controls the CCP-protected IVSEL bit in CTRLA. When set, the interrupt
 * vector table is placed at the start of the boot section; when cleared, at the
 * start of the application code section. Performed via the protected-write
 * sequence; configure during start-up before enabling global interrupts.
 *
 * @param boot true to place vectors in the boot section, false in the
 *             application section.
 */
static inline void intVectorsInBoot(bool boot)
{
	uint8_t ctrla = boot ? (CPUINT.CTRLA | CPUINT_IVSEL_bm)
	                     : (CPUINT.CTRLA & ~CPUINT_IVSEL_bm);
	ccp_write_io((void *)&CPUINT.CTRLA, ctrla);
}

/**
 * @brief Enable or disable the compact vector table.
 *
 * Controls the CCP-protected CVT bit in CTRLA. When enabled, the vector table
 * is reduced to three entries (NMI, the level-1 vector, and a single shared
 * level-0 vector), saving program space at the cost of dispatching all level-0
 * interrupts through one handler. Performed via the protected-write sequence;
 * configure during start-up before enabling global interrupts.
 *
 * @param enable true to enable the compact vector table, false to disable it.
 */
static inline void intCompactVectorTableEnable(bool enable)
{
	uint8_t ctrla = enable ? (CPUINT.CTRLA | CPUINT_CVT_bm)
	                       : (CPUINT.CTRLA & ~CPUINT_CVT_bm);
	ccp_write_io((void *)&CPUINT.CTRLA, ctrla);
}

// Execution status -----------------------------------------------------------
/**
 * @brief Read the interrupt execution-status flags.
 *
 * Returns the STATUS register masked to the valid flags. Test the result
 * against `INT_STATUS_LVL0EX` / `INT_STATUS_LVL1EX` / `INT_STATUS_NMIEX`.
 *
 * @return Bit mask of active execution-status flags (`intStatus_t`).
 */
static inline intStatus_t intGetStatus(void)
{
	return (intStatus_t)(CPUINT.STATUS & INT_STATUS_ALL);
}

/**
 * @brief Report whether a level-0 interrupt is currently executing.
 *
 * Reads the LVL0EX bit of STATUS.
 *
 * @return true if a level-0 interrupt handler is executing, false otherwise.
 */
static inline bool intLevel0Executing(void)
{
	return (CPUINT.STATUS & CPUINT_LVL0EX_bm) != 0;
}

/**
 * @brief Report whether the level-1 interrupt is currently executing.
 *
 * Reads the LVL1EX bit of STATUS.
 *
 * @return true if the level-1 interrupt handler is executing, false otherwise.
 */
static inline bool intLevel1Executing(void)
{
	return (CPUINT.STATUS & CPUINT_LVL1EX_bm) != 0;
}

/**
 * @brief Report whether a non-maskable interrupt is currently executing.
 *
 * Reads the NMIEX bit of STATUS.
 *
 * @return true if an NMI handler is executing, false otherwise.
 */
static inline bool intNMIExecuting(void)
{
	return (CPUINT.STATUS & CPUINT_NMIEX_bm) != 0;
}

/** @} */ // end of int_driver

#endif /* INT_H_ */
