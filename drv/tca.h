/**
 * @file tca.h
 * @brief TCA driver — inline accessors for the AVR-Dx Timer/Counter type A.
 *
 * Provides a thin, zero-overhead set of `static inline` functions that
 * manipulate the registers of a Timer/Counter type A (TCA) peripheral. Each
 * function operates on a caller-supplied `TCA_t *` so the same driver serves
 * any TCA instance.
 *
 * The TCA has two mutually exclusive operating modes selected by the SPLITM bit
 * (see `tcaSetSplitMode()`):
 *  - **Normal (single) mode**: one 16-bit timer/counter with three
 *    compare/waveform channels, buffered period and compare registers, count
 *    direction control, and a command strobe. The functions for this mode are
 *    named `tca...` and access the `SINGLE` view of the peripheral.
 *  - **Split mode**: the 16-bit timer is split into two independent 8-bit
 *    timers (a low and a high half), each with three compare outputs. The
 *    functions for this mode are named `tcaSplit...` and access the `SPLIT`
 *    view. Select the half with `tcaSplitTimer_t` and the output with
 *    `tcaChannel_t`.
 *
 * Switch modes only while the timer is disabled; the SPLITM bit changes how the
 * remaining registers are interpreted.
 *
 * ### Interrupt safety
 *
 * These accessors are **not** interrupt-safe. By design they are plain register
 * pokes with no internal interrupt masking, which keeps them zero-overhead and
 * lets the caller make a whole multi-register configuration sequence atomic as
 * a unit. Two hazards apply when the *same* TCA instance is accessed from both
 * main-line code and an ISR:
 *  - The 8-bit read-modify-write functions (e.g. `tcaSetClock()`,
 *    `tcaEnableCompare()`, `tcaEnableInterrupt()`) can lose a concurrent ISR
 *    update to the same register. (The CTRLE/CTRLF set/clear strobes —
 *    direction, lock-update, command, buffer-valid — are *not* read-modify-write
 *    and need no protection.)
 *  - In normal mode the 16-bit `CNT`/`PER`/`CMPn` registers are accessed as a
 *    low/high byte pair via the peripheral's `TEMP` register; an interrupt that
 *    performs its own 16-bit access on the same TCA between the two bytes
 *    corrupts the result. Split-mode registers are 8-bit and have no such
 *    hazard.
 *
 * Wrap affected accesses — ideally the entire logical sequence — in an
 * `ATOMIC_BLOCK` (see `<util/atomic.h>`). For reading `CNT`/`PER`/`CMPn` from
 * main-line code while an ISR also accesses them, the `tcaGetCountAtomic()`,
 * `tcaGetPeriodAtomic()`, and `tcaGetCompareAtomic()` variants do this for you.
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

#ifndef TCA_H_
#define TCA_H_

/** @addtogroup tca_driver
 * @{
 */

// Includes -------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <util/atomic.h>

// Data Types -----------------------------------------------------------------
/**
 * @brief Compare/waveform channel selector.
 *
 * The TCA has three compare channels in normal mode (CMP0–CMP2) and three
 * compare outputs per half in split mode (LCMP/HCMP 0–2).
 */
typedef enum
{
	TCA_CHANNEL0 = 0,	///< Compare channel 0
	TCA_CHANNEL1 = 1,	///< Compare channel 1
	TCA_CHANNEL2 = 2	///< Compare channel 2
} tcaChannel_t;

/**
 * @brief Split-mode timer-half selector.
 *
 * In split mode the 16-bit timer becomes two independent 8-bit timers.
 */
typedef enum
{
	TCA_SPLIT_LOW  = 0,	///< Low-byte timer
	TCA_SPLIT_HIGH = 1	///< High-byte timer
} tcaSplitTimer_t;

/**
 * @brief Normal-mode interrupt/flag source selector.
 *
 * Bit mask identifying which normal-mode interrupt sources to act on. Values
 * may be OR'd together. The bit positions match the SINGLE INTCTRL and INTFLAGS
 * registers, so the same mask applies to both.
 */
typedef enum
{
	TCA_INT_OVF  = TCA_SINGLE_OVF_bm,	///< Overflow/underflow interrupt
	TCA_INT_CMP0 = TCA_SINGLE_CMP0_bm,	///< Compare channel 0 interrupt
	TCA_INT_CMP1 = TCA_SINGLE_CMP1_bm,	///< Compare channel 1 interrupt
	TCA_INT_CMP2 = TCA_SINGLE_CMP2_bm,	///< Compare channel 2 interrupt
	TCA_INT_ALL  = TCA_SINGLE_OVF_bm | TCA_SINGLE_CMP0_bm |
	               TCA_SINGLE_CMP1_bm | TCA_SINGLE_CMP2_bm	///< All sources
} tcaInt_t;

/**
 * @brief Split-mode interrupt/flag source selector.
 *
 * Bit mask identifying which split-mode interrupt sources to act on. Values may
 * be OR'd together. The bit positions match the SPLIT INTCTRL and INTFLAGS
 * registers. Note that only the low-half compare channels raise interrupts.
 */
typedef enum
{
	TCA_SPLIT_INT_LUNF  = TCA_SPLIT_LUNF_bm,	///< Low-byte underflow interrupt
	TCA_SPLIT_INT_HUNF  = TCA_SPLIT_HUNF_bm,	///< High-byte underflow interrupt
	TCA_SPLIT_INT_LCMP0 = TCA_SPLIT_LCMP0_bm,	///< Low compare 0 interrupt
	TCA_SPLIT_INT_LCMP1 = TCA_SPLIT_LCMP1_bm,	///< Low compare 1 interrupt
	TCA_SPLIT_INT_LCMP2 = TCA_SPLIT_LCMP2_bm,	///< Low compare 2 interrupt
	TCA_SPLIT_INT_ALL   = TCA_SPLIT_LUNF_bm | TCA_SPLIT_HUNF_bm |
	                      TCA_SPLIT_LCMP0_bm | TCA_SPLIT_LCMP1_bm |
	                      TCA_SPLIT_LCMP2_bm	///< All sources
} tcaSplitInt_t;

// Mode Selection -------------------------------------------------------------
/**
 * @brief Select split or normal mode.
 *
 * Controls the SPLITM bit in CTRLD. Change this only while the timer is
 * disabled (`tcaDisable()`), as it changes how the other registers are
 * interpreted. After enabling split mode, use the `tcaSplit*` functions;
 * otherwise use the normal-mode functions.
 *
 * @param tca   Pointer to the TCA peripheral.
 * @param split true for split (dual 8-bit) mode, false for normal (16-bit) mode.
 */
static inline void tcaSetSplitMode(TCA_t *tca, bool split)
{
	if(split)
		tca->SINGLE.CTRLD |= TCA_SINGLE_SPLITM_bm;
	else
		tca->SINGLE.CTRLD &= ~TCA_SINGLE_SPLITM_bm;
}

// ===========================================================================
// Normal (single) mode
// ===========================================================================
// Run control ---------------------------------------------------------------
/**
 * @brief Enable the timer (start counting).
 *
 * Sets the ENABLE bit in CTRLA. Configure the mode, clock, and period before
 * enabling.
 *
 * @param tca Pointer to the TCA peripheral.
 */
static inline void tcaEnable(TCA_t *tca)
{
	tca->SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm;
}

/**
 * @brief Disable the timer (stop counting).
 *
 * Clears the ENABLE bit in CTRLA. The CNT register retains its value.
 *
 * @param tca Pointer to the TCA peripheral.
 */
static inline void tcaDisable(TCA_t *tca)
{
	tca->SINGLE.CTRLA &= ~TCA_SINGLE_ENABLE_bm;
}

/**
 * @brief Report whether the timer is enabled.
 *
 * Reads the ENABLE bit of CTRLA.
 *
 * @param tca Pointer to the TCA peripheral.
 * @return true if the timer is enabled, false otherwise.
 */
static inline bool tcaIsEnabled(const TCA_t *tca)
{
	return (tca->SINGLE.CTRLA & TCA_SINGLE_ENABLE_bm) != 0;
}

/**
 * @brief Select the counter clock prescaler.
 *
 * Writes the CLKSEL field of CTRLA (CLK_PER divided by 1…1024). Other CTRLA bits
 * — including the enable bit — are preserved.
 *
 * @param tca    Pointer to the TCA peripheral.
 * @param clkSel Clock prescaler group code (`TCA_SINGLE_CLKSEL_*_gc`).
 */
static inline void tcaSetClock(TCA_t *tca, TCA_SINGLE_CLKSEL_t clkSel)
{
	tca->SINGLE.CTRLA = (tca->SINGLE.CTRLA & ~TCA_SINGLE_CLKSEL_gm) | clkSel;
}

/**
 * @brief Enable or disable timer operation while the CPU is in standby sleep.
 *
 * Controls the RUNSTDBY bit in CTRLA.
 *
 * @param tca    Pointer to the TCA peripheral.
 * @param enable true to keep the timer running in standby, false otherwise.
 */
static inline void tcaRunStandby(TCA_t *tca, bool enable)
{
	if(enable)
		tca->SINGLE.CTRLA |= TCA_SINGLE_RUNSTDBY_bm;
	else
		tca->SINGLE.CTRLA &= ~TCA_SINGLE_RUNSTDBY_bm;
}

// Waveform mode -------------------------------------------------------------
/**
 * @brief Select the waveform generation mode.
 *
 * Writes the WGMODE field of CTRLB (normal, frequency, single-slope PWM, or one
 * of the dual-slope PWM variants). Other CTRLB bits — including the compare
 * output enables — are preserved.
 *
 * @param tca  Pointer to the TCA peripheral.
 * @param mode Waveform mode group code (`TCA_SINGLE_WGMODE_*_gc`).
 */
static inline void tcaSetMode(TCA_t *tca, TCA_SINGLE_WGMODE_t mode)
{
	tca->SINGLE.CTRLB = (tca->SINGLE.CTRLB & ~TCA_SINGLE_WGMODE_gm) | mode;
}

/**
 * @brief Enable or disable a compare channel's waveform output.
 *
 * Controls the CMPnEN bit in CTRLB for the selected channel, connecting the
 * channel's waveform to its peripheral pin.
 *
 * @param tca     Pointer to the TCA peripheral.
 * @param channel Compare channel (`tcaChannel_t`).
 * @param enable  true to enable the waveform output, false to disable it.
 */
static inline void tcaEnableCompare(TCA_t *tca, tcaChannel_t channel, bool enable)
{
	uint8_t mask = (uint8_t)(TCA_SINGLE_CMP0EN_bm << channel);
	if(enable)
		tca->SINGLE.CTRLB |= mask;
	else
		tca->SINGLE.CTRLB &= ~mask;
}

// Count direction -----------------------------------------------------------
/**
 * @brief Set the count direction.
 *
 * Uses the CTRLE set/clear strobe registers to update the DIR bit atomically
 * (no read-modify-write).
 *
 * @param tca  Pointer to the TCA peripheral.
 * @param down true to count down, false to count up.
 */
static inline void tcaSetCountDirection(TCA_t *tca, bool down)
{
	if(down)
		tca->SINGLE.CTRLESET = TCA_SINGLE_DIR_bm;
	else
		tca->SINGLE.CTRLECLR = TCA_SINGLE_DIR_bm;
}

/**
 * @brief Report whether the counter is counting down.
 *
 * Reads the DIR bit of CTRLE.
 *
 * @param tca Pointer to the TCA peripheral.
 * @return true if counting down, false if counting up.
 */
static inline bool tcaIsCountingDown(const TCA_t *tca)
{
	return (tca->SINGLE.CTRLECLR & TCA_SINGLE_DIR_bm) != 0;
}

// Command and update control ------------------------------------------------
/**
 * @brief Issue a timer command strobe.
 *
 * Writes the CMD field of CTRLESET to force an update, restart, or hard reset.
 * The command self-clears after execution. A hard reset is only accepted while
 * the timer is disabled.
 *
 * @param tca Pointer to the TCA peripheral.
 * @param cmd Command group code (`TCA_SINGLE_CMD_*_gc`).
 */
static inline void tcaCommand(TCA_t *tca, TCA_SINGLE_CMD_t cmd)
{
	tca->SINGLE.CTRLESET = cmd;
}

/**
 * @brief Lock or unlock buffered-register updates.
 *
 * Uses the CTRLE set/clear strobe to update the LUPD bit. While locked, the
 * period/compare buffer registers are not transferred to the active registers
 * at the update condition.
 *
 * @param tca  Pointer to the TCA peripheral.
 * @param lock true to lock updates, false to unlock.
 */
static inline void tcaLockUpdate(TCA_t *tca, bool lock)
{
	if(lock)
		tca->SINGLE.CTRLESET = TCA_SINGLE_LUPD_bm;
	else
		tca->SINGLE.CTRLECLR = TCA_SINGLE_LUPD_bm;
}

/**
 * @brief Enable or disable automatic lock of buffered updates.
 *
 * Controls the ALUPD bit in CTRLB.
 *
 * @param tca    Pointer to the TCA peripheral.
 * @param enable true to enable auto-lock-update, false to disable it.
 */
static inline void tcaSetAutoLockUpdate(TCA_t *tca, bool enable)
{
	if(enable)
		tca->SINGLE.CTRLB |= TCA_SINGLE_ALUPD_bm;
	else
		tca->SINGLE.CTRLB &= ~TCA_SINGLE_ALUPD_bm;
}

// Count, period, and compare registers --------------------------------------
/**
 * @brief Set the 16-bit count register (CNT).
 *
 * @param tca   Pointer to the TCA peripheral.
 * @param count Value to load into CNT.
 */
static inline void tcaSetCount(TCA_t *tca, uint16_t count)
{
	tca->SINGLE.CNT = count;
}

/**
 * @brief Read the 16-bit count register (CNT).
 *
 * @param tca Pointer to the TCA peripheral.
 * @return Current value of CNT.
 */
static inline uint16_t tcaGetCount(const TCA_t *tca)
{
	return tca->SINGLE.CNT;
}

/**
 * @brief Atomically read the 16-bit count register (CNT).
 *
 * Reads CNT with interrupts disabled so the low/high byte pair (and the shared
 * `TEMP` register) cannot be corrupted by an ISR that also performs a 16-bit
 * access on the same TCA. The previous global interrupt state is restored on
 * exit. Use this when `CNT`/`PER`/`CMPn` are accessed from both main-line code
 * and an ISR; otherwise `tcaGetCount()` is sufficient and cheaper.
 *
 * @param tca Pointer to the TCA peripheral.
 * @return Current value of CNT.
 */
static inline uint16_t tcaGetCountAtomic(const TCA_t *tca)
{
	uint16_t count;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		count = tca->SINGLE.CNT;
	}
	return count;
}

/**
 * @brief Set the 16-bit period register (PER), the counter TOP value.
 *
 * @param tca    Pointer to the TCA peripheral.
 * @param period Value to load into PER.
 */
static inline void tcaSetPeriod(TCA_t *tca, uint16_t period)
{
	tca->SINGLE.PER = period;
}

/**
 * @brief Read the 16-bit period register (PER).
 *
 * @param tca Pointer to the TCA peripheral.
 * @return Current value of PER.
 */
static inline uint16_t tcaGetPeriod(const TCA_t *tca)
{
	return tca->SINGLE.PER;
}

/**
 * @brief Atomically read the 16-bit period register (PER).
 *
 * See `tcaGetCountAtomic()` for the rationale.
 *
 * @param tca Pointer to the TCA peripheral.
 * @return Current value of PER.
 */
static inline uint16_t tcaGetPeriodAtomic(const TCA_t *tca)
{
	uint16_t period;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		period = tca->SINGLE.PER;
	}
	return period;
}

/**
 * @brief Set the buffered period register (PERBUF).
 *
 * The buffered value transfers to PER at the next update condition, allowing
 * glitch-free period changes while the timer runs.
 *
 * @param tca    Pointer to the TCA peripheral.
 * @param period Value to load into PERBUF.
 */
static inline void tcaSetPeriodBuffer(TCA_t *tca, uint16_t period)
{
	tca->SINGLE.PERBUF = period;
}

/**
 * @brief Set a 16-bit compare register (CMP0/CMP1/CMP2).
 *
 * In PWM modes the compare value sets the duty cycle of the channel.
 *
 * @param tca     Pointer to the TCA peripheral.
 * @param channel Compare channel (`tcaChannel_t`).
 * @param compare Value to load into the channel's compare register.
 */
static inline void tcaSetCompare(TCA_t *tca, tcaChannel_t channel, uint16_t compare)
{
	(&tca->SINGLE.CMP0)[channel] = compare;
}

/**
 * @brief Read a 16-bit compare register (CMP0/CMP1/CMP2).
 *
 * @param tca     Pointer to the TCA peripheral.
 * @param channel Compare channel (`tcaChannel_t`).
 * @return Current value of the channel's compare register.
 */
static inline uint16_t tcaGetCompare(const TCA_t *tca, tcaChannel_t channel)
{
	return (&tca->SINGLE.CMP0)[channel];
}

/**
 * @brief Atomically read a 16-bit compare register (CMP0/CMP1/CMP2).
 *
 * See `tcaGetCountAtomic()` for the rationale.
 *
 * @param tca     Pointer to the TCA peripheral.
 * @param channel Compare channel (`tcaChannel_t`).
 * @return Current value of the channel's compare register.
 */
static inline uint16_t tcaGetCompareAtomic(const TCA_t *tca, tcaChannel_t channel)
{
	uint16_t compare;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		compare = (&tca->SINGLE.CMP0)[channel];
	}
	return compare;
}

/**
 * @brief Set a buffered compare register (CMP0BUF/CMP1BUF/CMP2BUF).
 *
 * The buffered value transfers to the active compare register at the next
 * update condition, allowing glitch-free duty-cycle changes while the timer
 * runs.
 *
 * @param tca     Pointer to the TCA peripheral.
 * @param channel Compare channel (`tcaChannel_t`).
 * @param compare Value to load into the channel's compare buffer register.
 */
static inline void tcaSetCompareBuffer(TCA_t *tca, tcaChannel_t channel, uint16_t compare)
{
	(&tca->SINGLE.CMP0BUF)[channel] = compare;
}

// Interrupt control ---------------------------------------------------------
/**
 * @brief Enable one or more normal-mode interrupt sources.
 *
 * Sets the selected source bits in INTCTRL without disturbing other enabled
 * sources.
 *
 * @param tca  Pointer to the TCA peripheral.
 * @param mask Interrupt source(s) to enable (`tcaInt_t`, may be OR'd).
 */
static inline void tcaEnableInterrupt(TCA_t *tca, tcaInt_t mask)
{
	tca->SINGLE.INTCTRL |= mask;
}

/**
 * @brief Disable one or more normal-mode interrupt sources.
 *
 * Clears the selected source bits in INTCTRL.
 *
 * @param tca  Pointer to the TCA peripheral.
 * @param mask Interrupt source(s) to disable (`tcaInt_t`, may be OR'd).
 */
static inline void tcaDisableInterrupt(TCA_t *tca, tcaInt_t mask)
{
	tca->SINGLE.INTCTRL &= ~mask;
}

/**
 * @brief Read the pending normal-mode interrupt flags.
 *
 * Returns INTFLAGS masked to the valid interrupt sources.
 *
 * @param tca Pointer to the TCA peripheral.
 * @return Bit mask of pending interrupt flags (`tcaInt_t`).
 */
static inline tcaInt_t tcaGetInterruptFlags(const TCA_t *tca)
{
	return (tcaInt_t)(tca->SINGLE.INTFLAGS & TCA_INT_ALL);
}

/**
 * @brief Clear one or more pending normal-mode interrupt flags.
 *
 * Flags are cleared by writing a one to them; only the requested bits are
 * written so unrelated pending flags are preserved.
 *
 * @param tca  Pointer to the TCA peripheral.
 * @param mask Interrupt flag(s) to clear (`tcaInt_t`, may be OR'd).
 */
static inline void tcaClearInterruptFlags(TCA_t *tca, tcaInt_t mask)
{
	tca->SINGLE.INTFLAGS = mask;
}

// Event control -------------------------------------------------------------
/**
 * @brief Enable or disable counting on event input A.
 *
 * Controls the CNTAEI bit in EVCTRL.
 *
 * @param tca    Pointer to the TCA peripheral.
 * @param enable true to enable event input A, false to disable it.
 */
static inline void tcaEnableEventCountA(TCA_t *tca, bool enable)
{
	if(enable)
		tca->SINGLE.EVCTRL |= TCA_SINGLE_CNTAEI_bm;
	else
		tca->SINGLE.EVCTRL &= ~TCA_SINGLE_CNTAEI_bm;
}

/**
 * @brief Select the action for event input A.
 *
 * Writes the EVACTA field of EVCTRL (count or direction-control behavior).
 *
 * @param tca    Pointer to the TCA peripheral.
 * @param action Event action group code (`TCA_SINGLE_EVACTA_*_gc`).
 */
static inline void tcaSetEventActionA(TCA_t *tca, TCA_SINGLE_EVACTA_t action)
{
	tca->SINGLE.EVCTRL = (tca->SINGLE.EVCTRL & ~TCA_SINGLE_EVACTA_gm) | action;
}

/**
 * @brief Enable or disable counting on event input B.
 *
 * Controls the CNTBEI bit in EVCTRL.
 *
 * @param tca    Pointer to the TCA peripheral.
 * @param enable true to enable event input B, false to disable it.
 */
static inline void tcaEnableEventCountB(TCA_t *tca, bool enable)
{
	if(enable)
		tca->SINGLE.EVCTRL |= TCA_SINGLE_CNTBEI_bm;
	else
		tca->SINGLE.EVCTRL &= ~TCA_SINGLE_CNTBEI_bm;
}

/**
 * @brief Select the action for event input B.
 *
 * Writes the EVACTB field of EVCTRL (no action, direction control, or restart
 * behavior).
 *
 * @param tca    Pointer to the TCA peripheral.
 * @param action Event action group code (`TCA_SINGLE_EVACTB_*_gc`).
 */
static inline void tcaSetEventActionB(TCA_t *tca, TCA_SINGLE_EVACTB_t action)
{
	tca->SINGLE.EVCTRL = (tca->SINGLE.EVCTRL & ~TCA_SINGLE_EVACTB_gm) | action;
}

// Debug control -------------------------------------------------------------
/**
 * @brief Enable or disable timer operation while halted in debug.
 *
 * Controls the DBGRUN bit in DBGCTRL.
 *
 * @param tca    Pointer to the TCA peripheral.
 * @param enable true to keep the timer running while the debugger has the CPU
 *               halted, false to freeze it.
 */
static inline void tcaDebugRun(TCA_t *tca, bool enable)
{
	if(enable)
		tca->SINGLE.DBGCTRL |= TCA_SINGLE_DBGRUN_bm;
	else
		tca->SINGLE.DBGCTRL &= ~TCA_SINGLE_DBGRUN_bm;
}

// ===========================================================================
// Split mode (dual 8-bit)
// ===========================================================================
// Run control ---------------------------------------------------------------
/**
 * @brief Enable the timer in split mode.
 *
 * Sets the ENABLE bit in CTRLA. Enable split mode (`tcaSetSplitMode()`) and
 * configure the clock and periods before enabling.
 *
 * @param tca Pointer to the TCA peripheral.
 */
static inline void tcaSplitEnable(TCA_t *tca)
{
	tca->SPLIT.CTRLA |= TCA_SPLIT_ENABLE_bm;
}

/**
 * @brief Disable the timer in split mode.
 *
 * Clears the ENABLE bit in CTRLA.
 *
 * @param tca Pointer to the TCA peripheral.
 */
static inline void tcaSplitDisable(TCA_t *tca)
{
	tca->SPLIT.CTRLA &= ~TCA_SPLIT_ENABLE_bm;
}

/**
 * @brief Select the counter clock prescaler in split mode.
 *
 * Writes the CLKSEL field of CTRLA. Both halves share this prescaler. Other
 * CTRLA bits — including the enable bit — are preserved.
 *
 * @param tca    Pointer to the TCA peripheral.
 * @param clkSel Clock prescaler group code (`TCA_SPLIT_CLKSEL_*_gc`).
 */
static inline void tcaSplitSetClock(TCA_t *tca, TCA_SPLIT_CLKSEL_t clkSel)
{
	tca->SPLIT.CTRLA = (tca->SPLIT.CTRLA & ~TCA_SPLIT_CLKSEL_gm) | clkSel;
}

/**
 * @brief Enable or disable split-mode operation while in standby sleep.
 *
 * Controls the RUNSTDBY bit in CTRLA.
 *
 * @param tca    Pointer to the TCA peripheral.
 * @param enable true to keep the timer running in standby, false otherwise.
 */
static inline void tcaSplitRunStandby(TCA_t *tca, bool enable)
{
	if(enable)
		tca->SPLIT.CTRLA |= TCA_SPLIT_RUNSTDBY_bm;
	else
		tca->SPLIT.CTRLA &= ~TCA_SPLIT_RUNSTDBY_bm;
}

// Compare output enable -----------------------------------------------------
/**
 * @brief Enable or disable a compare output in split mode.
 *
 * Controls the L/HCMPnEN bit in CTRLB for the selected half and channel.
 *
 * @param tca     Pointer to the TCA peripheral.
 * @param timer   Timer half (`tcaSplitTimer_t`).
 * @param channel Compare channel (`tcaChannel_t`).
 * @param enable  true to enable the waveform output, false to disable it.
 */
static inline void tcaSplitEnableCompare(TCA_t *tca, tcaSplitTimer_t timer, tcaChannel_t channel, bool enable)
{
	uint8_t base = (timer == TCA_SPLIT_HIGH) ? TCA_SPLIT_HCMP0EN_bm : TCA_SPLIT_LCMP0EN_bm;
	uint8_t mask = (uint8_t)(base << channel);
	if(enable)
		tca->SPLIT.CTRLB |= mask;
	else
		tca->SPLIT.CTRLB &= ~mask;
}

// Command -------------------------------------------------------------------
/**
 * @brief Issue a command strobe in split mode.
 *
 * Writes the CMDEN and CMD fields of CTRLESET so the command applies to both
 * halves. The command self-clears after execution.
 *
 * @param tca Pointer to the TCA peripheral.
 * @param cmd Command group code (`TCA_SPLIT_CMD_*_gc`).
 */
static inline void tcaSplitCommand(TCA_t *tca, TCA_SPLIT_CMD_t cmd)
{
	tca->SPLIT.CTRLESET = TCA_SPLIT_CMDEN_BOTH_gc | cmd;
}

// Count, period, and compare registers --------------------------------------
/**
 * @brief Set the 8-bit count register of a split timer half (LCNT/HCNT).
 *
 * @param tca   Pointer to the TCA peripheral.
 * @param timer Timer half (`tcaSplitTimer_t`).
 * @param count Value to load into the half's count register.
 */
static inline void tcaSplitSetCount(TCA_t *tca, tcaSplitTimer_t timer, uint8_t count)
{
	(&tca->SPLIT.LCNT)[timer] = count;
}

/**
 * @brief Read the 8-bit count register of a split timer half (LCNT/HCNT).
 *
 * @param tca   Pointer to the TCA peripheral.
 * @param timer Timer half (`tcaSplitTimer_t`).
 * @return Current value of the half's count register.
 */
static inline uint8_t tcaSplitGetCount(const TCA_t *tca, tcaSplitTimer_t timer)
{
	return (&tca->SPLIT.LCNT)[timer];
}

/**
 * @brief Set the 8-bit period register of a split timer half (LPER/HPER).
 *
 * Each split half counts down from its period value to zero.
 *
 * @param tca    Pointer to the TCA peripheral.
 * @param timer  Timer half (`tcaSplitTimer_t`).
 * @param period Value to load into the half's period register.
 */
static inline void tcaSplitSetPeriod(TCA_t *tca, tcaSplitTimer_t timer, uint8_t period)
{
	(&tca->SPLIT.LPER)[timer] = period;
}

/**
 * @brief Read the 8-bit period register of a split timer half (LPER/HPER).
 *
 * @param tca   Pointer to the TCA peripheral.
 * @param timer Timer half (`tcaSplitTimer_t`).
 * @return Current value of the half's period register.
 */
static inline uint8_t tcaSplitGetPeriod(const TCA_t *tca, tcaSplitTimer_t timer)
{
	return (&tca->SPLIT.LPER)[timer];
}

/**
 * @brief Set an 8-bit compare register in split mode.
 *
 * Addresses the L/HCMPn register for the selected half and channel.
 *
 * @param tca     Pointer to the TCA peripheral.
 * @param timer   Timer half (`tcaSplitTimer_t`).
 * @param channel Compare channel (`tcaChannel_t`).
 * @param compare Value to load into the compare register.
 */
static inline void tcaSplitSetCompare(TCA_t *tca, tcaSplitTimer_t timer, tcaChannel_t channel, uint8_t compare)
{
	(&tca->SPLIT.LCMP0)[channel * 2 + timer] = compare;
}

/**
 * @brief Read an 8-bit compare register in split mode.
 *
 * @param tca     Pointer to the TCA peripheral.
 * @param timer   Timer half (`tcaSplitTimer_t`).
 * @param channel Compare channel (`tcaChannel_t`).
 * @return Current value of the compare register.
 */
static inline uint8_t tcaSplitGetCompare(const TCA_t *tca, tcaSplitTimer_t timer, tcaChannel_t channel)
{
	return (&tca->SPLIT.LCMP0)[channel * 2 + timer];
}

// Interrupt control ---------------------------------------------------------
/**
 * @brief Enable one or more split-mode interrupt sources.
 *
 * Sets the selected source bits in INTCTRL without disturbing other enabled
 * sources.
 *
 * @param tca  Pointer to the TCA peripheral.
 * @param mask Interrupt source(s) to enable (`tcaSplitInt_t`, may be OR'd).
 */
static inline void tcaSplitEnableInterrupt(TCA_t *tca, tcaSplitInt_t mask)
{
	tca->SPLIT.INTCTRL |= mask;
}

/**
 * @brief Disable one or more split-mode interrupt sources.
 *
 * Clears the selected source bits in INTCTRL.
 *
 * @param tca  Pointer to the TCA peripheral.
 * @param mask Interrupt source(s) to disable (`tcaSplitInt_t`, may be OR'd).
 */
static inline void tcaSplitDisableInterrupt(TCA_t *tca, tcaSplitInt_t mask)
{
	tca->SPLIT.INTCTRL &= ~mask;
}

/**
 * @brief Read the pending split-mode interrupt flags.
 *
 * Returns INTFLAGS masked to the valid interrupt sources.
 *
 * @param tca Pointer to the TCA peripheral.
 * @return Bit mask of pending interrupt flags (`tcaSplitInt_t`).
 */
static inline tcaSplitInt_t tcaSplitGetInterruptFlags(const TCA_t *tca)
{
	return (tcaSplitInt_t)(tca->SPLIT.INTFLAGS & TCA_SPLIT_INT_ALL);
}

/**
 * @brief Clear one or more pending split-mode interrupt flags.
 *
 * Flags are cleared by writing a one to them; only the requested bits are
 * written so unrelated pending flags are preserved.
 *
 * @param tca  Pointer to the TCA peripheral.
 * @param mask Interrupt flag(s) to clear (`tcaSplitInt_t`, may be OR'd).
 */
static inline void tcaSplitClearInterruptFlags(TCA_t *tca, tcaSplitInt_t mask)
{
	tca->SPLIT.INTFLAGS = mask;
}

// Debug control -------------------------------------------------------------
/**
 * @brief Enable or disable split-mode operation while halted in debug.
 *
 * Controls the DBGRUN bit in DBGCTRL.
 *
 * @param tca    Pointer to the TCA peripheral.
 * @param enable true to keep the timer running while the debugger has the CPU
 *               halted, false to freeze it.
 */
static inline void tcaSplitDebugRun(TCA_t *tca, bool enable)
{
	if(enable)
		tca->SPLIT.DBGCTRL |= TCA_SPLIT_DBGRUN_bm;
	else
		tca->SPLIT.DBGCTRL &= ~TCA_SPLIT_DBGRUN_bm;
}

/** @} */ // end of tca_driver

#endif /* TCA_H_ */
