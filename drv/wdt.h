/**
 * @file wdt.h
 * @brief WDT driver — inline accessors for the AVR-Dx watchdog timer.
 *
 * Provides a thin, zero-overhead set of `static inline` functions that
 * manipulate the registers of the Watchdog Timer (WDT). The AVR-Dx has a single
 * watchdog, so these functions operate directly on the global `WDT` register set
 * rather than on a caller-supplied pointer.
 *
 * The watchdog resets the device if it is not periodically cleared. The driver
 * sets the time-out period, optionally enables windowed mode (which also faults
 * if the watchdog is cleared too *early*), clears the watchdog (`wdtReset()`),
 * and locks the configuration.
 *
 * @note The CTRLA register and the STATUS LOCK bit are protected by the
 *       Configuration Change Protection (CCP) mechanism; the setters below
 *       perform the protected write via avr-libc's `ccp_write_io()`.
 *
 * @note The watchdog runs in its own clock domain. A write to CTRLA takes
 *       several cycles to synchronize, during which STATUS.SYNCBUSY is set and
 *       further CTRLA writes are ignored. The configuration setters poll
 *       SYNCBUSY and block until a previous write has synchronized, so they are
 *       safe to call back to back.
 *
 * @note The watchdog cannot be reconfigured once locked — either by
 *       `wdtLock()` or by the WDT fuses. The setters have no effect in that
 *       state.
 *
 * ### Interrupt safety
 *
 * `wdtReset()` is a single `wdr` instruction and is safe to call from anywhere,
 * including ISRs. The configuration setters are read-modify-write combined with
 * a CCP-timed protected write; `ccp_write_io()` performs the timed sequence with
 * interrupts disabled. The watchdog is normally configured once at start-up.
 * There is no byte-pair (`TEMP`) hazard.
 *
 * The bit-mask (`_bm`), group-mask (`_gm`), and group-code (`_gc`) symbols
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

#ifndef WDT_H_
#define WDT_H_

/** @addtogroup wdt_driver
 * @{
 */

// Includes -------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/cpufunc.h>

// Inline Functions -----------------------------------------------------------
// Watchdog clear -------------------------------------------------------------
/**
 * @brief Clear (kick) the watchdog timer.
 *
 * Executes the `wdr` instruction, resetting the watchdog counter so the time-out
 * does not elapse. Call this periodically within the configured period (and,
 * in windowed mode, not before the closed window has elapsed). Safe to call from
 * any context, including an ISR.
 */
static inline void wdtReset(void)
{
	__asm__ __volatile__("wdr");
}

// Synchronization ------------------------------------------------------------
/**
 * @brief Test whether a CTRLA write is still synchronizing.
 *
 * Reads the SYNCBUSY bit of STATUS.
 *
 * @return true if a previous CTRLA write is still synchronizing, false
 *         otherwise.
 */
static inline bool wdtSyncBusy(void)
{
	return (WDT.STATUS & WDT_SYNCBUSY_bm) != 0;
}

/**
 * @brief Block until a pending CTRLA write has synchronized.
 *
 * Spins while STATUS.SYNCBUSY is set.
 */
static inline void wdtWaitSync(void)
{
	while(WDT.STATUS & WDT_SYNCBUSY_bm)
		;
}

// Configuration --------------------------------------------------------------
/**
 * @brief Set the watchdog time-out period.
 *
 * Writes the PERIOD field of CTRLA via the protected-write sequence, choosing
 * the time-out duration. A non-OFF period enables the watchdog;
 * `WDT_PERIOD_OFF_gc` disables it. The WINDOW field is preserved. Blocks until
 * any pending CTRLA synchronization completes before writing.
 *
 * @param period Time-out period group code (`WDT_PERIOD_*_gc`).
 */
static inline void wdtSetPeriod(WDT_PERIOD_t period)
{
	wdtWaitSync();
	ccp_write_io((void *)&WDT.CTRLA, (WDT.CTRLA & ~WDT_PERIOD_gm) | period);
}

/**
 * @brief Read the watchdog time-out period.
 *
 * @return The PERIOD field of CTRLA (`WDT_PERIOD_t`).
 */
static inline WDT_PERIOD_t wdtGetPeriod(void)
{
	return (WDT_PERIOD_t)(WDT.CTRLA & WDT_PERIOD_gm);
}

/**
 * @brief Set the watchdog closed-window period.
 *
 * Writes the WINDOW field of CTRLA via the protected-write sequence. A non-OFF
 * window enables windowed mode, in which clearing the watchdog *before* the
 * window elapses also triggers a reset; `WDT_WINDOW_OFF_gc` selects normal mode.
 * The PERIOD field is preserved. Blocks until any pending CTRLA synchronization
 * completes before writing.
 *
 * @param window Closed-window period group code (`WDT_WINDOW_*_gc`).
 */
static inline void wdtSetWindow(WDT_WINDOW_t window)
{
	wdtWaitSync();
	ccp_write_io((void *)&WDT.CTRLA, (WDT.CTRLA & ~WDT_WINDOW_gm) | window);
}

/**
 * @brief Read the watchdog closed-window period.
 *
 * @return The WINDOW field of CTRLA (`WDT_WINDOW_t`).
 */
static inline WDT_WINDOW_t wdtGetWindow(void)
{
	return (WDT_WINDOW_t)(WDT.CTRLA & WDT_WINDOW_gm);
}

/**
 * @brief Enable the watchdog in normal (non-windowed) mode.
 *
 * Writes CTRLA with the given period and the window turned off, via the
 * protected-write sequence. Blocks until any pending CTRLA synchronization
 * completes before writing.
 *
 * @param period Time-out period group code (`WDT_PERIOD_*_gc`, non-OFF).
 */
static inline void wdtEnable(WDT_PERIOD_t period)
{
	wdtWaitSync();
	ccp_write_io((void *)&WDT.CTRLA, period | WDT_WINDOW_OFF_gc);
}

/**
 * @brief Disable the watchdog.
 *
 * Writes CTRLA to turn both the period and window off, via the protected-write
 * sequence. Has no effect if the watchdog is locked. Blocks until any pending
 * CTRLA synchronization completes before writing.
 */
static inline void wdtDisable(void)
{
	wdtWaitSync();
	ccp_write_io((void *)&WDT.CTRLA, WDT_PERIOD_OFF_gc | WDT_WINDOW_OFF_gc);
}

// Configuration lock ---------------------------------------------------------
/**
 * @brief Report whether the watchdog configuration is locked.
 *
 * Reads the LOCK bit of STATUS. When set, CTRLA cannot be changed until the next
 * reset.
 *
 * @return true if the configuration is locked, false otherwise.
 */
static inline bool wdtIsLocked(void)
{
	return (WDT.STATUS & WDT_LOCK_bm) != 0;
}

/**
 * @brief Lock the watchdog configuration until the next reset.
 *
 * Sets the LOCK bit in STATUS via the protected-write sequence. Once locked, the
 * period and window cannot be changed until a reset occurs. Blocks until any
 * pending CTRLA synchronization completes before writing.
 */
static inline void wdtLock(void)
{
	wdtWaitSync();
	ccp_write_io((void *)&WDT.STATUS, WDT_LOCK_bm);
}

/** @} */ // end of wdt_driver

#endif /* WDT_H_ */
