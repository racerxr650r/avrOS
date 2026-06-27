/**
 * @file rtc.h
 * @brief RTC driver — inline accessors for the AVR-Dx Real-Time Counter.
 *
 * Provides a thin, zero-overhead set of `static inline` functions that
 * manipulate the registers of the Real-Time Counter (RTC) peripheral. The
 * AVR-Dx has a single RTC instance, so these functions operate directly on the
 * global `RTC` register set rather than on a caller-supplied pointer.
 *
 * The peripheral contains two independent functional blocks:
 *  - The **RTC counter**: a 16-bit up-counter clocked from a low-frequency
 *    source, with period (PER) and compare (CMP) registers and overflow /
 *    compare-match interrupts.
 *  - The **PIT** (Periodic Interrupt Timer): a separate divider off the same
 *    clock that generates a periodic interrupt at one of a fixed set of
 *    intervals. PIT functions are prefixed `rtcPit`.
 *
 * @note The RTC runs in its own clock domain. Writes to the count-domain
 *       registers (CTRLA, CNT, PER, CMP, and the PIT's PITCTRLA) must not occur
 *       while a previous write is still synchronizing. The setter functions
 *       below poll the relevant busy flag and block until synchronization
 *       completes, so they are safe to call back to back. The clock source
 *       (CLKSEL) may only be changed while the RTC is disabled.
 *
 * ### Interrupt safety
 *
 * Separately from the clock-domain synchronization described above, these
 * accessors are **not** interrupt-safe. The usual register-access hazards apply
 * when the RTC is touched from both main-line code and an interrupt handler
 * (e.g. the RTC overflow/compare ISR or the PIT ISR):
 *  - The 8-bit read-modify-write functions (e.g. `rtcEnable()`,
 *    `rtcEnableInterrupt()`, `rtcPitEnable()`) can lose a concurrent ISR update
 *    to the same register.
 *  - The 16-bit `CNT`/`PER`/`CMP` registers are accessed as a low/high byte pair
 *    via the RTC's `TEMP` register; an interrupt that performs its own 16-bit
 *    RTC access between the two bytes corrupts the result.
 *
 * The caller is responsible for serializing affected accesses with an
 * `ATOMIC_BLOCK` (see `<util/atomic.h>`). Note that wrapping a *blocking* setter
 * (one that waits on a sync flag) in an `ATOMIC_BLOCK` holds interrupts off for
 * the entire synchronization period — up to a few RTC clock cycles, which at
 * 32.768 kHz / 1.024 kHz can be tens of microseconds — so prefer to serialize
 * only the specific access that races rather than a blocking write. For reading
 * `CNT`/`PER`/`CMP` from main-line code while an ISR also accesses them, the
 * `rtcGetCountAtomic()`, `rtcGetPeriodAtomic()`, and `rtcGetCompareAtomic()`
 * variants perform the masking for you and do not block on synchronization.
 *
 * The group-code (`_gc`), bit-mask (`_bm`), and bit-position (`_bp`) symbols
 * referenced here are supplied by `<avr/io.h>` for the selected device.
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

#ifndef RTC_H_
#define RTC_H_

/** @addtogroup rtc_driver
 * @{
 */

// Includes -------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <util/atomic.h>

// Data Types -----------------------------------------------------------------
/**
 * @brief RTC counter interrupt/flag source selector.
 *
 * Bit mask identifying which RTC counter interrupt sources to act on when
 * enabling, disabling, testing, or clearing interrupts. Values may be OR'd
 * together. The bit positions match the layout of the RTC INTCTRL and INTFLAGS
 * registers, so the same mask applies to both. These are distinct from the PIT
 * interrupt (see `rtcPitGetInterruptFlag()`).
 */
typedef enum
{
	RTC_INT_OVF = RTC_OVF_bm,	///< Overflow interrupt (CNT wrapped past PER)
	RTC_INT_CMP = RTC_CMP_bm,	///< Compare-match interrupt (CNT == CMP)
	RTC_INT_ALL = RTC_OVF_bm | RTC_CMP_bm	///< Both interrupt sources
} rtcInt_t;

/**
 * @brief RTC synchronization (busy) flag selector.
 *
 * Bit mask identifying the count-domain registers whose writes synchronize
 * across the clock-domain boundary. The bit positions match the RTC STATUS
 * register. Use with `rtcSyncBusy()` / `rtcWaitSync()` to poll or wait for a
 * pending write to complete.
 */
typedef enum
{
	RTC_SYNC_CTRLA = RTC_CTRLABUSY_bm,	///< CTRLA write synchronizing
	RTC_SYNC_CNT   = RTC_CNTBUSY_bm,	///< CNT write synchronizing
	RTC_SYNC_PER   = RTC_PERBUSY_bm,	///< PER write synchronizing
	RTC_SYNC_CMP   = RTC_CMPBUSY_bm,	///< CMP write synchronizing
	RTC_SYNC_ALL   = RTC_CTRLABUSY_bm | RTC_CNTBUSY_bm |
	                 RTC_PERBUSY_bm | RTC_CMPBUSY_bm	///< Any of the above
} rtcSync_t;

// Inline Functions -----------------------------------------------------------
// Synchronization -----------------------------------------------------------
/**
 * @brief Test whether a count-domain register write is still synchronizing.
 *
 * Reads the RTC STATUS register and returns whether any of the requested busy
 * flags are set.
 *
 * @param mask Synchronization flag(s) to test (`rtcSync_t`, may be OR'd).
 * @return true if any selected register is still busy, false otherwise.
 */
static inline bool rtcSyncBusy(rtcSync_t mask)
{
	return (RTC.STATUS & mask) != 0;
}

/**
 * @brief Block until the selected count-domain register writes have completed.
 *
 * Spins while any of the requested busy flags remain set in RTC STATUS.
 *
 * @param mask Synchronization flag(s) to wait on (`rtcSync_t`, may be OR'd).
 */
static inline void rtcWaitSync(rtcSync_t mask)
{
	while(RTC.STATUS & mask)
		;
}

// Clock and prescaler -------------------------------------------------------
/**
 * @brief Select the RTC clock source.
 *
 * Writes the CLKSEL register (OSC32K, OSC1K, external 32.768 kHz crystal, or
 * external clock). This shared clock also drives the PIT.
 *
 * @note The clock source may only be changed while the RTC is disabled
 *       (see `rtcDisable()`).
 *
 * @param clkSel Clock source group code (`RTC_CLKSEL_*_gc`).
 */
static inline void rtcSetClock(RTC_CLKSEL_t clkSel)
{
	RTC.CLKSEL = clkSel;
}

/**
 * @brief Set the RTC counter prescaler.
 *
 * Writes the PRESCALER field of CTRLA, dividing the RTC clock that feeds the
 * counter. Other CTRLA bits — including the enable bit — are preserved. Blocks
 * until any pending CTRLA synchronization completes before writing.
 *
 * @param prescaler Prescaler group code (`RTC_PRESCALER_*_gc`).
 */
static inline void rtcSetPrescaler(RTC_PRESCALER_t prescaler)
{
	rtcWaitSync(RTC_SYNC_CTRLA);
	RTC.CTRLA = (RTC.CTRLA & ~RTC_PRESCALER_gm) | prescaler;
}

// Run control ---------------------------------------------------------------
/**
 * @brief Enable the RTC counter.
 *
 * Sets the RTCEN bit in CTRLA. Configure the clock source, prescaler, and
 * period before enabling. Blocks until any pending CTRLA synchronization
 * completes before writing.
 */
static inline void rtcEnable(void)
{
	rtcWaitSync(RTC_SYNC_CTRLA);
	RTC.CTRLA |= RTC_RTCEN_bm;
}

/**
 * @brief Disable the RTC counter.
 *
 * Clears the RTCEN bit in CTRLA. Blocks until any pending CTRLA synchronization
 * completes before writing.
 */
static inline void rtcDisable(void)
{
	rtcWaitSync(RTC_SYNC_CTRLA);
	RTC.CTRLA &= ~RTC_RTCEN_bm;
}

/**
 * @brief Report whether the RTC counter is enabled.
 *
 * Reads the RTCEN bit of CTRLA.
 *
 * @return true if the RTC counter is enabled, false otherwise.
 */
static inline bool rtcIsEnabled(void)
{
	return (RTC.CTRLA & RTC_RTCEN_bm) != 0;
}

/**
 * @brief Enable or disable RTC operation while the CPU is in standby sleep.
 *
 * Controls the RUNSTDBY bit in CTRLA. Blocks until any pending CTRLA
 * synchronization completes before writing.
 *
 * @param enable true to keep the RTC running in standby, false otherwise.
 */
static inline void rtcRunStandby(bool enable)
{
	rtcWaitSync(RTC_SYNC_CTRLA);
	if(enable)
		RTC.CTRLA |= RTC_RUNSTDBY_bm;
	else
		RTC.CTRLA &= ~RTC_RUNSTDBY_bm;
}

// Counter registers ---------------------------------------------------------
/**
 * @brief Set the 16-bit counter register (CNT).
 *
 * Blocks until any pending CNT synchronization completes before writing.
 *
 * @param count Value to load into CNT.
 */
static inline void rtcSetCount(uint16_t count)
{
	rtcWaitSync(RTC_SYNC_CNT);
	RTC.CNT = count;
}

/**
 * @brief Read the 16-bit counter register (CNT).
 *
 * @return Current value of CNT.
 */
static inline uint16_t rtcGetCount(void)
{
	return RTC.CNT;
}

/**
 * @brief Atomically read the 16-bit counter register (CNT).
 *
 * Reads CNT with interrupts disabled so the low/high byte pair (and the shared
 * `TEMP` register) cannot be corrupted by an ISR that also performs a 16-bit
 * RTC access. The previous global interrupt state is restored on exit. Does not
 * block on clock-domain synchronization. Use this variant when `CNT`/`PER`/`CMP`
 * are read from main-line code while an ISR also accesses them; otherwise
 * `rtcGetCount()` is sufficient and cheaper.
 *
 * @return Current value of CNT.
 */
static inline uint16_t rtcGetCountAtomic(void)
{
	uint16_t count;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		count = RTC.CNT;
	}
	return count;
}

/**
 * @brief Set the 16-bit period register (PER), the counter TOP value.
 *
 * The counter wraps to zero and raises the overflow interrupt when it passes
 * PER. Blocks until any pending PER synchronization completes before writing.
 *
 * @param period Value to load into PER.
 */
static inline void rtcSetPeriod(uint16_t period)
{
	rtcWaitSync(RTC_SYNC_PER);
	RTC.PER = period;
}

/**
 * @brief Read the 16-bit period register (PER).
 *
 * @return Current value of PER.
 */
static inline uint16_t rtcGetPeriod(void)
{
	return RTC.PER;
}

/**
 * @brief Atomically read the 16-bit period register (PER).
 *
 * Reads PER with interrupts disabled so the low/high byte pair (and the shared
 * `TEMP` register) cannot be corrupted by an ISR that also performs a 16-bit
 * RTC access. The previous global interrupt state is restored on exit. Does not
 * block on clock-domain synchronization. See `rtcGetCountAtomic()` for when to
 * prefer this variant.
 *
 * @return Current value of PER.
 */
static inline uint16_t rtcGetPeriodAtomic(void)
{
	uint16_t period;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		period = RTC.PER;
	}
	return period;
}

/**
 * @brief Set the 16-bit compare register (CMP).
 *
 * A compare-match interrupt is raised when CNT equals CMP. Blocks until any
 * pending CMP synchronization completes before writing.
 *
 * @param compare Value to load into CMP.
 */
static inline void rtcSetCompare(uint16_t compare)
{
	rtcWaitSync(RTC_SYNC_CMP);
	RTC.CMP = compare;
}

/**
 * @brief Read the 16-bit compare register (CMP).
 *
 * @return Current value of CMP.
 */
static inline uint16_t rtcGetCompare(void)
{
	return RTC.CMP;
}

/**
 * @brief Atomically read the 16-bit compare register (CMP).
 *
 * Reads CMP with interrupts disabled so the low/high byte pair (and the shared
 * `TEMP` register) cannot be corrupted by an ISR that also performs a 16-bit
 * RTC access. The previous global interrupt state is restored on exit. Does not
 * block on clock-domain synchronization. See `rtcGetCountAtomic()` for when to
 * prefer this variant.
 *
 * @return Current value of CMP.
 */
static inline uint16_t rtcGetCompareAtomic(void)
{
	uint16_t compare;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		compare = RTC.CMP;
	}
	return compare;
}

// Interrupt control ---------------------------------------------------------
/**
 * @brief Enable one or more RTC counter interrupt sources.
 *
 * Sets the selected source bits in the INTCTRL register without disturbing
 * other enabled sources.
 *
 * @param mask Interrupt source(s) to enable (`rtcInt_t`, may be OR'd).
 */
static inline void rtcEnableInterrupt(rtcInt_t mask)
{
	RTC.INTCTRL |= mask;
}

/**
 * @brief Disable one or more RTC counter interrupt sources.
 *
 * Clears the selected source bits in the INTCTRL register.
 *
 * @param mask Interrupt source(s) to disable (`rtcInt_t`, may be OR'd).
 */
static inline void rtcDisableInterrupt(rtcInt_t mask)
{
	RTC.INTCTRL &= ~mask;
}

/**
 * @brief Read the pending RTC counter interrupt flags.
 *
 * Returns the INTFLAGS register masked to the valid interrupt sources. Test
 * the result against `RTC_INT_OVF` / `RTC_INT_CMP`.
 *
 * @return Bit mask of pending interrupt flags (`rtcInt_t`).
 */
static inline rtcInt_t rtcGetInterruptFlags(void)
{
	return (rtcInt_t)(RTC.INTFLAGS & RTC_INT_ALL);
}

/**
 * @brief Clear one or more pending RTC counter interrupt flags.
 *
 * The INTFLAGS bits are cleared by writing a one to them; this function writes
 * only the requested bits so that unrelated pending flags are preserved.
 *
 * @param mask Interrupt flag(s) to clear (`rtcInt_t`, may be OR'd).
 */
static inline void rtcClearInterruptFlags(rtcInt_t mask)
{
	RTC.INTFLAGS = mask;
}

// Crystal error correction --------------------------------------------------
/**
 * @brief Enable or disable automatic crystal frequency error correction.
 *
 * Controls the CORREN bit in CTRLA. When enabled, the RTC applies the
 * calibration value programmed via `rtcSetCalibration()`. Blocks until any
 * pending CTRLA synchronization completes before writing.
 *
 * @param enable true to enable correction, false to disable it.
 */
static inline void rtcEnableCorrection(bool enable)
{
	rtcWaitSync(RTC_SYNC_CTRLA);
	if(enable)
		RTC.CTRLA |= RTC_CORREN_bm;
	else
		RTC.CTRLA &= ~RTC_CORREN_bm;
}

/**
 * @brief Program the crystal frequency error-correction value.
 *
 * Writes the CALIB register. The correction adjusts the effective RTC
 * frequency by periodically adding or skipping clock cycles. Correction must
 * also be enabled via `rtcEnableCorrection()`.
 *
 * @param fast  true to speed the clock up (positive sign), false to slow it
 *              down (negative sign).
 * @param error Magnitude of the correction in 0.5-cycle steps (0–127).
 */
static inline void rtcSetCalibration(bool fast, uint8_t error)
{
	RTC.CALIB = (fast ? RTC_SIGN_bm : 0) | (error & RTC_ERROR_gm);
}

// Debug control -------------------------------------------------------------
/**
 * @brief Enable or disable RTC counter operation while halted in debug.
 *
 * Controls the DBGRUN bit in DBGCTRL.
 *
 * @param enable true to keep the RTC counter running while the debugger has the
 *               CPU halted, false to freeze it.
 */
static inline void rtcDebugRun(bool enable)
{
	if(enable)
		RTC.DBGCTRL |= RTC_DBGRUN_bm;
	else
		RTC.DBGCTRL &= ~RTC_DBGRUN_bm;
}

// PIT — Periodic Interrupt Timer --------------------------------------------
/**
 * @brief Test whether a PIT control write is still synchronizing.
 *
 * Reads the PITSTATUS register CTRLBUSY flag.
 *
 * @return true if PITCTRLA is still synchronizing, false otherwise.
 */
static inline bool rtcPitSyncBusy(void)
{
	return (RTC.PITSTATUS & RTC_CTRLBUSY_bm) != 0;
}

/**
 * @brief Block until a pending PIT control write has completed.
 *
 * Spins while the PITSTATUS CTRLBUSY flag is set.
 */
static inline void rtcPitWaitSync(void)
{
	while(RTC.PITSTATUS & RTC_CTRLBUSY_bm)
		;
}

/**
 * @brief Set the PIT interrupt period.
 *
 * Writes the PERIOD field of PITCTRLA, selecting how many RTC clock cycles
 * elapse between periodic interrupts. Other PITCTRLA bits — including the PIT
 * enable bit — are preserved. Blocks until any pending PIT synchronization
 * completes before writing.
 *
 * @param period PIT period group code (`RTC_PERIOD_*_gc`).
 */
static inline void rtcPitSetPeriod(RTC_PERIOD_t period)
{
	rtcPitWaitSync();
	RTC.PITCTRLA = (RTC.PITCTRLA & ~RTC_PERIOD_gm) | period;
}

/**
 * @brief Enable the PIT.
 *
 * Sets the PITEN bit in PITCTRLA. Set the period via `rtcPitSetPeriod()`
 * before enabling. Blocks until any pending PIT synchronization completes
 * before writing.
 */
static inline void rtcPitEnable(void)
{
	rtcPitWaitSync();
	RTC.PITCTRLA |= RTC_PITEN_bm;
}

/**
 * @brief Disable the PIT.
 *
 * Clears the PITEN bit in PITCTRLA. Blocks until any pending PIT
 * synchronization completes before writing.
 */
static inline void rtcPitDisable(void)
{
	rtcPitWaitSync();
	RTC.PITCTRLA &= ~RTC_PITEN_bm;
}

/**
 * @brief Report whether the PIT is enabled.
 *
 * Reads the PITEN bit of PITCTRLA.
 *
 * @return true if the PIT is enabled, false otherwise.
 */
static inline bool rtcPitIsEnabled(void)
{
	return (RTC.PITCTRLA & RTC_PITEN_bm) != 0;
}

/**
 * @brief Enable or disable the PIT periodic interrupt.
 *
 * Controls the PI bit in PITINTCTRL.
 *
 * @param enable true to enable the periodic interrupt, false to disable it.
 */
static inline void rtcPitEnableInterrupt(bool enable)
{
	if(enable)
		RTC.PITINTCTRL |= RTC_PI_bm;
	else
		RTC.PITINTCTRL &= ~RTC_PI_bm;
}

/**
 * @brief Test whether the PIT periodic interrupt flag is pending.
 *
 * Reads the PI bit of PITINTFLAGS.
 *
 * @return true if the PIT periodic interrupt is pending, false otherwise.
 */
static inline bool rtcPitGetInterruptFlag(void)
{
	return (RTC.PITINTFLAGS & RTC_PI_bm) != 0;
}

/**
 * @brief Clear the pending PIT periodic interrupt flag.
 *
 * The flag is cleared by writing a one to the PI bit of PITINTFLAGS.
 */
static inline void rtcPitClearInterruptFlag(void)
{
	RTC.PITINTFLAGS = RTC_PI_bm;
}

/**
 * @brief Enable or disable PIT operation while halted in debug.
 *
 * Controls the DBGRUN bit in PITDBGCTRL.
 *
 * @param enable true to keep the PIT running while the debugger has the CPU
 *               halted, false to freeze it.
 */
static inline void rtcPitDebugRun(bool enable)
{
	if(enable)
		RTC.PITDBGCTRL |= RTC_DBGRUN_bm;
	else
		RTC.PITDBGCTRL &= ~RTC_DBGRUN_bm;
}

/** @} */ // end of rtc_driver

#endif /* RTC_H_ */
