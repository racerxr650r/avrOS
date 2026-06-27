/**
 * @file slp.h
 * @brief Sleep driver — inline accessors for the AVR-Dx sleep controller.
 *
 * Provides a thin, zero-overhead set of `static inline` functions that
 * manipulate the registers of the Sleep Controller (SLPCTRL). The AVR-Dx has a
 * single sleep controller, so these functions operate directly on the global
 * `SLPCTRL` register set rather than on a caller-supplied pointer.
 *
 * The driver selects the sleep mode (idle, standby, power-down), enables sleep,
 * executes the `sleep` instruction to enter the selected mode, and configures
 * the voltage-regulator performance/low-leakage options in VREGCTRL. A
 * convenience `slpSleep()` performs the enable/sleep/disable cycle in one call.
 *
 * After a `sleep` instruction the CPU resumes on any enabled interrupt; ensure
 * the desired wake source and global interrupts are enabled before sleeping.
 *
 * @note The SLPCTRL registers are not protected by the Configuration Change
 *       Protection mechanism, so no special unlock sequence is required.
 *
 * ### Interrupt safety
 *
 * `slpSetMode()`, `slpEnable()`, and the VREGCTRL accessors are 8-bit
 * read-modify-write operations; they are normally configured from a single
 * context (the idle loop) and need no masking. There is no byte-pair (`TEMP`)
 * hazard. The classic enable/sleep/disable pattern in `slpSleep()` mirrors
 * avr-libc's `sleep_mode()`.
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

#ifndef SLP_H_
#define SLP_H_

/** @addtogroup slp_driver
 * @{
 */

// Includes -------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>

// Inline Functions -----------------------------------------------------------
// Sleep mode -----------------------------------------------------------------
/**
 * @brief Select the sleep mode.
 *
 * Writes the SMODE field of CTRLA (idle, standby, or power-down). The deeper the
 * mode, the more peripherals are stopped and the lower the power. The sleep
 * enable (SEN) bit is preserved.
 *
 * @param mode Sleep mode group code (`SLPCTRL_SMODE_*_gc`).
 */
static inline void slpSetMode(SLPCTRL_SMODE_t mode)
{
	SLPCTRL.CTRLA = (SLPCTRL.CTRLA & ~SLPCTRL_SMODE_gm) | mode;
}

/**
 * @brief Read the selected sleep mode.
 *
 * @return The SMODE field of CTRLA (`SLPCTRL_SMODE_t`).
 */
static inline SLPCTRL_SMODE_t slpGetMode(void)
{
	return (SLPCTRL_SMODE_t)(SLPCTRL.CTRLA & SLPCTRL_SMODE_gm);
}

/**
 * @brief Enable or disable sleep.
 *
 * Controls the SEN bit in CTRLA. The CPU only enters sleep on a `sleep`
 * instruction while this bit is set; keeping it clear except around an actual
 * sleep avoids unintended sleep.
 *
 * @param enable true to allow sleep, false to prevent it.
 */
static inline void slpEnable(bool enable)
{
	if(enable)
		SLPCTRL.CTRLA |= SLPCTRL_SEN_bm;
	else
		SLPCTRL.CTRLA &= ~SLPCTRL_SEN_bm;
}

/**
 * @brief Execute the sleep instruction.
 *
 * Puts the CPU into the configured sleep mode. Sleep must be enabled
 * (`slpEnable(true)`) for this to take effect; the CPU resumes on the next
 * enabled interrupt.
 */
static inline void slpEnter(void)
{
	__asm__ __volatile__("sleep" ::: "memory");
}

/**
 * @brief Enter a sleep mode, then disable sleep on wake.
 *
 * Selects @p mode, enables sleep, executes the sleep instruction, and clears the
 * sleep-enable bit after the CPU wakes — the same safe enable/sleep/disable
 * cycle as avr-libc's `sleep_mode()`.
 *
 * @param mode Sleep mode group code (`SLPCTRL_SMODE_*_gc`).
 */
static inline void slpSleep(SLPCTRL_SMODE_t mode)
{
	slpSetMode(mode);
	slpEnable(true);
	slpEnter();
	slpEnable(false);
}

// Voltage regulator ----------------------------------------------------------
/**
 * @brief Select the voltage-regulator performance mode in sleep.
 *
 * Writes the PMODE field of VREGCTRL, trading wake-up latency against power in
 * the low-power sleep modes.
 *
 * @param mode Performance mode group code (`SLPCTRL_PMODE_*_gc`).
 */
static inline void slpSetPerformanceMode(SLPCTRL_PMODE_t mode)
{
	SLPCTRL.VREGCTRL = (SLPCTRL.VREGCTRL & ~SLPCTRL_PMODE_gm) | mode;
}

/**
 * @brief Enable or disable the high-temperature low-leakage option.
 *
 * Controls the HTLLEN bit in VREGCTRL, reducing leakage current in standby and
 * power-down at high temperatures.
 *
 * @param enable true to enable high-temperature low-leakage, false to disable.
 */
static inline void slpHighTempLowLeakage(bool enable)
{
	if(enable)
		SLPCTRL.VREGCTRL |= SLPCTRL_HTLLEN_bm;
	else
		SLPCTRL.VREGCTRL &= ~SLPCTRL_HTLLEN_bm;
}

/** @} */ // end of slp_driver

#endif /* SLP_H_ */
