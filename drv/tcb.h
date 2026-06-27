/**
 * @file tcb.h
 * @brief TCB driver — inline accessors for the AVR-Dx Timer/Counter type B.
 *
 * Provides a thin, zero-overhead set of `static inline` functions that
 * manipulate the registers of a Timer/Counter type B (TCB) peripheral. Each
 * function operates on a caller-supplied `TCB_t *` so the same driver serves
 * any TCB instance (TCB0, TCB1, TCB2, ...). The driver exposes mode and clock
 * configuration, run control, the count and compare/capture registers,
 * interrupt and event control, and status/debug control.
 *
 * The group-code (`_gc`), bit-mask (`_bm`), and bit-position (`_bp`) symbols
 * referenced here are supplied by `<avr/io.h>` for the selected device.
 *
 * ### Interrupt safety
 *
 * These accessors are **not** interrupt-safe. By design they are plain register
 * pokes with no internal interrupt masking — this keeps them zero-overhead and,
 * more importantly, lets the caller make a whole multi-register configuration
 * sequence atomic as a unit rather than one operation at a time (per-call
 * masking would not protect the sequence and would only add jitter). Two
 * hazards apply when the *same* TCB instance is accessed from both main-line
 * code and an ISR:
 *  - The 8-bit read-modify-write functions (e.g. `tcbSetMode()`, `tcbEnable()`,
 *    `tcbEnableInterrupt()`) can lose a concurrent ISR update to the same
 *    register.
 *  - The 16-bit `CNT`/`CCMP` registers are accessed as a low/high byte pair via
 *    the peripheral's `TEMP` register; an interrupt that performs its own
 *    16-bit access on the same TCB between the two bytes corrupts the result.
 *    (Each TCB instance has its own `TEMP`, so distinct instances cannot
 *    corrupt one another.)
 *
 * The caller is responsible for wrapping affected accesses — ideally the entire
 * logical sequence — in an `ATOMIC_BLOCK` (see `<util/atomic.h>`), as the system
 * tick setup in `sys.c` does. For the common case of reading `CNT`/`CCMP` from
 * main-line code while an ISR also accesses them, the `tcbGetCountAtomic()` and
 * `tcbGetCaptureAtomic()` variants perform the masking for you.
 *
 * Created: 6/21/2026
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

#ifndef TCB_H_
#define TCB_H_

/** @addtogroup tcb_driver
 * @{
 */

// Includes -------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <util/atomic.h>

// Data Types -----------------------------------------------------------------
/**
 * @brief TCB interrupt/flag source selector.
 *
 * Bit mask identifying which TCB interrupt sources to act on when enabling,
 * disabling, testing, or clearing interrupts. Values may be OR'd together to
 * address both sources in a single call. The bit positions match the layout
 * of the TCB INTCTRL and INTFLAGS registers, so the same mask applies to
 * both.
 */
typedef enum
{
	TCB_INT_CAPT = TCB_CAPT_bm,	///< Capture/timeout interrupt (CAPT)
	TCB_INT_OVF  = TCB_OVF_bm,	///< Overflow interrupt (OVF, async modes only)
	TCB_INT_ALL  = TCB_CAPT_bm | TCB_OVF_bm	///< Both interrupt sources
} tcbInt_t;

// Inline Functions -----------------------------------------------------------
// Mode and clock configuration ----------------------------------------------
/**
 * @brief Select the counter operating mode.
 *
 * Writes the CNTMODE field of CTRLB, choosing the waveform/capture behavior of
 * the timer (periodic interrupt, timeout, input capture, single shot, 8-bit
 * PWM, ...). Other bits in CTRLB are preserved.
 *
 * @param tcb  Pointer to the TCB peripheral.
 * @param mode Counter mode group code (`TCB_CNTMODE_*_gc`).
 */
static inline void tcbSetMode(TCB_t *tcb, TCB_CNTMODE_t mode)
{
	tcb->CTRLB = (tcb->CTRLB & ~TCB_CNTMODE_gm) | mode;
}

/**
 * @brief Select the counter clock source.
 *
 * Writes the CLKSEL field of CTRLA (peripheral clock, peripheral clock / 2,
 * the TCA clock, or an event edge). Other bits in CTRLA — including the enable
 * bit — are preserved.
 *
 * @param tcb    Pointer to the TCB peripheral.
 * @param clkSel Clock source group code (`TCB_CLKSEL_*_gc`).
 */
static inline void tcbSetClock(TCB_t *tcb, TCB_CLKSEL_t clkSel)
{
	tcb->CTRLA = (tcb->CTRLA & ~TCB_CLKSEL_gm) | clkSel;
}

// Run control ---------------------------------------------------------------
/**
 * @brief Enable the timer (start counting).
 *
 * Sets the ENABLE bit in CTRLA. Configure the mode, clock source, and
 * compare/capture value before enabling.
 *
 * @param tcb Pointer to the TCB peripheral.
 */
static inline void tcbEnable(TCB_t *tcb)
{
	tcb->CTRLA |= TCB_ENABLE_bm;
}

/**
 * @brief Disable the timer (stop counting).
 *
 * Clears the ENABLE bit in CTRLA. The CNT register retains its value.
 *
 * @param tcb Pointer to the TCB peripheral.
 */
static inline void tcbDisable(TCB_t *tcb)
{
	tcb->CTRLA &= ~TCB_ENABLE_bm;
}

/**
 * @brief Enable or disable timer operation while the CPU is in standby sleep.
 *
 * Controls the RUNSTDBY bit in CTRLA.
 *
 * @param tcb    Pointer to the TCB peripheral.
 * @param enable true to keep the timer running in standby, false otherwise.
 */
static inline void tcbRunStandby(TCB_t *tcb, bool enable)
{
	if(enable)
		tcb->CTRLA |= TCB_RUNSTDBY_bm;
	else
		tcb->CTRLA &= ~TCB_RUNSTDBY_bm;
}

/**
 * @brief Report whether the timer is currently running.
 *
 * Reads the RUN bit of the STATUS register.
 *
 * @param tcb Pointer to the TCB peripheral.
 * @return true if the counter is running, false otherwise.
 */
static inline bool tcbIsRunning(const TCB_t *tcb)
{
	return (tcb->STATUS & TCB_RUN_bm) != 0;
}

// Count and compare/capture registers ---------------------------------------
/**
 * @brief Set the 16-bit count register (CNT).
 *
 * @param tcb   Pointer to the TCB peripheral.
 * @param count Value to load into CNT.
 */
static inline void tcbSetCount(TCB_t *tcb, uint16_t count)
{
	tcb->CNT = count;
}

/**
 * @brief Read the 16-bit count register (CNT).
 *
 * @param tcb Pointer to the TCB peripheral.
 * @return Current value of CNT.
 */
static inline uint16_t tcbGetCount(const TCB_t *tcb)
{
	return tcb->CNT;
}

/**
 * @brief Atomically read the 16-bit count register (CNT).
 *
 * Reads CNT with interrupts disabled so the low/high byte pair (and the shared
 * `TEMP` register) cannot be corrupted by an ISR that also performs a 16-bit
 * access on the same TCB. The previous global interrupt state is restored on
 * exit. Use this variant when `CNT`/`CCMP` of the same instance are touched
 * from both main-line code and an interrupt handler; otherwise `tcbGetCount()`
 * is sufficient and cheaper.
 *
 * @param tcb Pointer to the TCB peripheral.
 * @return Current value of CNT.
 */
static inline uint16_t tcbGetCountAtomic(const TCB_t *tcb)
{
	uint16_t count;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		count = tcb->CNT;
	}
	return count;
}

/**
 * @brief Set the 16-bit compare/capture register (CCMP).
 *
 * In timer modes CCMP holds the TOP value (period); in capture modes it
 * receives the captured count and is normally read rather than written.
 *
 * @param tcb  Pointer to the TCB peripheral.
 * @param ccmp Value to load into CCMP.
 */
static inline void tcbSetCompare(TCB_t *tcb, uint16_t ccmp)
{
	tcb->CCMP = ccmp;
}

/**
 * @brief Read the 16-bit compare/capture register (CCMP).
 *
 * @param tcb Pointer to the TCB peripheral.
 * @return Current value of CCMP (the captured value in capture modes).
 */
static inline uint16_t tcbGetCapture(const TCB_t *tcb)
{
	return tcb->CCMP;
}

/**
 * @brief Atomically read the 16-bit compare/capture register (CCMP).
 *
 * Reads CCMP with interrupts disabled so the low/high byte pair (and the shared
 * `TEMP` register) cannot be corrupted by an ISR that also performs a 16-bit
 * access on the same TCB. The previous global interrupt state is restored on
 * exit. Use this variant when `CNT`/`CCMP` of the same instance are touched
 * from both main-line code and an interrupt handler; otherwise
 * `tcbGetCapture()` is sufficient and cheaper.
 *
 * @param tcb Pointer to the TCB peripheral.
 * @return Current value of CCMP (the captured value in capture modes).
 */
static inline uint16_t tcbGetCaptureAtomic(const TCB_t *tcb)
{
	uint16_t ccmp;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		ccmp = tcb->CCMP;
	}
	return ccmp;
}

// Interrupt control ---------------------------------------------------------
/**
 * @brief Enable one or more TCB interrupt sources.
 *
 * Sets the selected source bits in the INTCTRL register without disturbing
 * other enabled sources.
 *
 * @param tcb  Pointer to the TCB peripheral.
 * @param mask Interrupt source(s) to enable (`tcbInt_t`, may be OR'd).
 */
static inline void tcbEnableInterrupt(TCB_t *tcb, tcbInt_t mask)
{
	tcb->INTCTRL |= mask;
}

/**
 * @brief Disable one or more TCB interrupt sources.
 *
 * Clears the selected source bits in the INTCTRL register.
 *
 * @param tcb  Pointer to the TCB peripheral.
 * @param mask Interrupt source(s) to disable (`tcbInt_t`, may be OR'd).
 */
static inline void tcbDisableInterrupt(TCB_t *tcb, tcbInt_t mask)
{
	tcb->INTCTRL &= ~mask;
}

/**
 * @brief Read the pending interrupt flags.
 *
 * Returns the raw INTFLAGS register masked to the valid interrupt sources.
 * Test the result against `TCB_INT_CAPT` / `TCB_INT_OVF`.
 *
 * @param tcb Pointer to the TCB peripheral.
 * @return Bit mask of pending interrupt flags (`tcbInt_t`).
 */
static inline tcbInt_t tcbGetInterruptFlags(const TCB_t *tcb)
{
	return (tcbInt_t)(tcb->INTFLAGS & TCB_INT_ALL);
}

/**
 * @brief Clear one or more pending interrupt flags.
 *
 * The TCB INTFLAGS bits are cleared by writing a one to them; this function
 * writes only the requested bits so that unrelated pending flags are
 * preserved.
 *
 * @param tcb  Pointer to the TCB peripheral.
 * @param mask Interrupt flag(s) to clear (`tcbInt_t`, may be OR'd).
 */
static inline void tcbClearInterruptFlags(TCB_t *tcb, tcbInt_t mask)
{
	tcb->INTFLAGS = mask;
}

// Event control -------------------------------------------------------------
/**
 * @brief Enable or disable the capture event input.
 *
 * Controls the CAPTEI bit in EVCTRL, which routes the configured event channel
 * into the timer for input-capture and event-counting modes.
 *
 * @param tcb    Pointer to the TCB peripheral.
 * @param enable true to enable the event input, false to disable it.
 */
static inline void tcbEventInputEnable(TCB_t *tcb, bool enable)
{
	if(enable)
		tcb->EVCTRL |= TCB_CAPTEI_bm;
	else
		tcb->EVCTRL &= ~TCB_CAPTEI_bm;
}

/**
 * @brief Select the active edge of the capture event.
 *
 * Controls the EDGE bit in EVCTRL. The exact meaning of each edge depends on
 * the selected counter mode (see the device datasheet).
 *
 * @param tcb  Pointer to the TCB peripheral.
 * @param edge true to select the alternate (EDGE=1) event edge, false for the
 *             default (EDGE=0) edge.
 */
static inline void tcbEventEdge(TCB_t *tcb, bool edge)
{
	if(edge)
		tcb->EVCTRL |= TCB_EDGE_bm;
	else
		tcb->EVCTRL &= ~TCB_EDGE_bm;
}

/**
 * @brief Enable or disable the input capture noise-cancellation filter.
 *
 * Controls the FILTER bit in EVCTRL, which debounces the event input over four
 * sample periods before it reaches the timer.
 *
 * @param tcb    Pointer to the TCB peripheral.
 * @param enable true to enable the filter, false to disable it.
 */
static inline void tcbEventFilter(TCB_t *tcb, bool enable)
{
	if(enable)
		tcb->EVCTRL |= TCB_FILTER_bm;
	else
		tcb->EVCTRL &= ~TCB_FILTER_bm;
}

// Pin output ----------------------------------------------------------------
/**
 * @brief Enable or disable the compare/capture waveform pin output.
 *
 * Controls the CCMPEN bit in CTRLB, which drives the timer waveform onto the
 * associated peripheral pin (used in PWM and timeout/output modes).
 *
 * @param tcb    Pointer to the TCB peripheral.
 * @param enable true to drive the output pin, false to release it.
 */
static inline void tcbOutputEnable(TCB_t *tcb, bool enable)
{
	if(enable)
		tcb->CTRLB |= TCB_CCMPEN_bm;
	else
		tcb->CTRLB &= ~TCB_CCMPEN_bm;
}

// Debug control -------------------------------------------------------------
/**
 * @brief Enable or disable timer operation while the debugger has the CPU
 *        halted.
 *
 * Controls the DBGRUN bit in DBGCTRL.
 *
 * @param tcb    Pointer to the TCB peripheral.
 * @param enable true to keep the timer running while halted in debug, false to
 *               freeze it.
 */
static inline void tcbDebugRun(TCB_t *tcb, bool enable)
{
	if(enable)
		tcb->DBGCTRL |= TCB_DBGRUN_bm;
	else
		tcb->DBGCTRL &= ~TCB_DBGRUN_bm;
}

/** @} */ // end of tcb_driver

#endif /* TCB_H_ */
