/**
 * @file clk.h
 * @brief Clock driver — inline accessors for the AVR-Dx clock controller.
 *
 * Provides a thin, zero-overhead set of `static inline` functions that
 * manipulate the registers of the Clock Controller (CLKCTRL). The AVR-Dx has a
 * single clock controller, so these functions operate directly on the global
 * `CLKCTRL` register set rather than on a caller-supplied pointer.
 *
 * The driver covers main-clock source selection and prescaler, the internal
 * high-frequency oscillator (frequency, auto-tune, run-in-standby), the internal
 * and external 32.768 kHz oscillators, the PLL, the clock-output pin, the
 * configuration lock, and the read-only oscillator-status flags.
 *
 * @note Most CLKCTRL configuration registers are protected by the Configuration
 *       Change Protection (CCP) mechanism. The setters below perform the
 *       protected write via avr-libc's `ccp_write_io()`; the status register is
 *       a plain read. Once the configuration is locked with `clkLock()`, the
 *       protected registers cannot be changed until the next reset.
 *
 * ### Interrupt safety
 *
 * The clock is normally configured once during start-up before global
 * interrupts are enabled. The read-modify-write setters (e.g. `clkSetSource()`)
 * are not interrupt-safe; serialize with an `ATOMIC_BLOCK` (see
 * `<util/atomic.h>`) if reconfiguring while interrupts are live. All registers
 * are 8-bit; there is no byte-pair (`TEMP`) hazard.
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

#ifndef CLK_H_
#define CLK_H_

/** @addtogroup clk_driver
 * @{
 */

// Includes -------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/cpufunc.h>

// Data Types -----------------------------------------------------------------
/**
 * @brief Oscillator/clock status flag selector.
 *
 * Bit mask identifying the read-only status flags in MCLKSTATUS. Values may be
 * OR'd together when testing the result of `clkGetStatus()`. A set oscillator
 * flag means that source is running and stable.
 */
typedef enum
{
	CLK_STATUS_CHANGING = CLKCTRL_SOSC_bm,		///< Main clock source is changing
	CLK_STATUS_OSCHF    = CLKCTRL_OSCHFS_bm,	///< Internal HF oscillator stable
	CLK_STATUS_OSC32K   = CLKCTRL_OSC32KS_bm,	///< Internal 32.768 kHz oscillator stable
	CLK_STATUS_XOSC32K  = CLKCTRL_XOSC32KS_bm,	///< External 32.768 kHz crystal stable
	CLK_STATUS_EXTCLK   = CLKCTRL_EXTS_bm,		///< External clock stable
	CLK_STATUS_PLL      = CLKCTRL_PLLS_bm		///< PLL stable
} clkStatus_t;

// Inline Functions -----------------------------------------------------------
// Main clock source and prescaler --------------------------------------------
/**
 * @brief Select the main clock source.
 *
 * Writes the CLKSEL field of MCLKCTRLA (internal HF/32 kHz oscillator, external
 * crystal, or external clock). The clock-output (CLKOUT) bit is preserved.
 * Performed via the protected-write sequence.
 *
 * @param source Clock source group code (`CLKCTRL_CLKSEL_*_gc`).
 */
static inline void clkSetSource(CLKCTRL_CLKSEL_t source)
{
	ccp_write_io((void *)&CLKCTRL.MCLKCTRLA,
	             (CLKCTRL.MCLKCTRLA & ~CLKCTRL_CLKSEL_gm) | source);
}

/**
 * @brief Read the selected main clock source.
 *
 * @return The CLKSEL field of MCLKCTRLA (`CLKCTRL_CLKSEL_t`).
 */
static inline CLKCTRL_CLKSEL_t clkGetSource(void)
{
	return (CLKCTRL_CLKSEL_t)(CLKCTRL.MCLKCTRLA & CLKCTRL_CLKSEL_gm);
}

/**
 * @brief Enable or disable the main clock output on the CLKOUT pin.
 *
 * Controls the CLKOUT bit in MCLKCTRLA. The CLKSEL field is preserved.
 * Performed via the protected-write sequence.
 *
 * @param enable true to drive the clock onto the CLKOUT pin, false to disable.
 */
static inline void clkClockOut(bool enable)
{
	uint8_t value = enable ? (CLKCTRL.MCLKCTRLA | CLKCTRL_CLKOUT_bm)
	                       : (CLKCTRL.MCLKCTRLA & ~CLKCTRL_CLKOUT_bm);
	ccp_write_io((void *)&CLKCTRL.MCLKCTRLA, value);
}

/**
 * @brief Configure the main clock prescaler.
 *
 * Writes MCLKCTRLB with the prescaler division factor and enable bit. When
 * disabled, the main clock equals the source frequency. Performed via the
 * protected-write sequence.
 *
 * @param division Prescaler division group code (`CLKCTRL_PDIV_*_gc`).
 * @param enable   true to enable the prescaler, false to bypass it.
 */
static inline void clkSetPrescaler(CLKCTRL_PDIV_t division, bool enable)
{
	uint8_t value = division | (enable ? CLKCTRL_PEN_bm : 0);
	ccp_write_io((void *)&CLKCTRL.MCLKCTRLB, value);
}

/**
 * @brief Report whether the main clock prescaler is enabled.
 *
 * Reads the PEN bit of MCLKCTRLB.
 *
 * @return true if the prescaler is enabled, false otherwise.
 */
static inline bool clkPrescalerEnabled(void)
{
	return (CLKCTRL.MCLKCTRLB & CLKCTRL_PEN_bm) != 0;
}

/**
 * @brief Read the main clock prescaler division factor.
 *
 * Returns the PDIV field of MCLKCTRLB.
 *
 * @return The prescaler division group code (`CLKCTRL_PDIV_t`).
 */
static inline CLKCTRL_PDIV_t clkGetPrescaler(void)
{
	return (CLKCTRL_PDIV_t)(CLKCTRL.MCLKCTRLB & CLKCTRL_PDIV_gm);
}

// Configuration lock ---------------------------------------------------------
/**
 * @brief Lock the clock configuration until the next reset.
 *
 * Sets the LOCKEN bit in MCLKLOCK via the protected-write sequence. Once locked,
 * the CCP-protected clock registers cannot be modified until a reset occurs.
 */
static inline void clkLock(void)
{
	ccp_write_io((void *)&CLKCTRL.MCLKLOCK, CLKCTRL_LOCKEN_bm);
}

/**
 * @brief Report whether the clock configuration is locked.
 *
 * Reads the LOCKEN bit of MCLKLOCK.
 *
 * @return true if the configuration is locked, false otherwise.
 */
static inline bool clkIsLocked(void)
{
	return (CLKCTRL.MCLKLOCK & CLKCTRL_LOCKEN_bm) != 0;
}

// Status ---------------------------------------------------------------------
/**
 * @brief Read the oscillator/clock status flags.
 *
 * Returns the MCLKSTATUS register. Test against `CLK_STATUS_OSCHF`,
 * `CLK_STATUS_CHANGING`, etc.
 *
 * @return Bit mask of active status flags (`clkStatus_t`).
 */
static inline clkStatus_t clkGetStatus(void)
{
	return (clkStatus_t)CLKCTRL.MCLKSTATUS;
}

/**
 * @brief Test whether one or more status flags are set.
 *
 * @param mask Status flag(s) to test (`clkStatus_t`, may be OR'd).
 * @return true if any selected flag is set, false otherwise.
 */
static inline bool clkStatusReady(clkStatus_t mask)
{
	return (CLKCTRL.MCLKSTATUS & mask) != 0;
}

// Internal high-frequency oscillator -----------------------------------------
/**
 * @brief Set the internal high-frequency oscillator frequency.
 *
 * Writes the FRQSEL field of OSCHFCTRLA. The auto-tune and run-standby bits are
 * preserved. Performed via the protected-write sequence.
 *
 * @param frequency Frequency-select group code (`CLKCTRL_FRQSEL_*_gc`).
 */
static inline void clkSetOscHFFrequency(CLKCTRL_FRQSEL_t frequency)
{
	ccp_write_io((void *)&CLKCTRL.OSCHFCTRLA,
	             (CLKCTRL.OSCHFCTRLA & ~CLKCTRL_FRQSEL_gm) | frequency);
}

/**
 * @brief Read the internal high-frequency oscillator frequency selection.
 *
 * @return The FRQSEL field of OSCHFCTRLA (`CLKCTRL_FRQSEL_t`).
 */
static inline CLKCTRL_FRQSEL_t clkGetOscHFFrequency(void)
{
	return (CLKCTRL_FRQSEL_t)(CLKCTRL.OSCHFCTRLA & CLKCTRL_FRQSEL_gm);
}

/**
 * @brief Enable or disable auto-tuning of the internal HF oscillator.
 *
 * Controls the AUTOTUNE bit in OSCHFCTRLA, which tunes the HF oscillator against
 * a running external 32.768 kHz crystal. Performed via the protected-write
 * sequence.
 *
 * @param enable true to enable auto-tune, false to disable it.
 */
static inline void clkOscHFAutotune(bool enable)
{
	uint8_t value = enable ? (CLKCTRL.OSCHFCTRLA | CLKCTRL_AUTOTUNE_bm)
	                       : (CLKCTRL.OSCHFCTRLA & ~CLKCTRL_AUTOTUNE_bm);
	ccp_write_io((void *)&CLKCTRL.OSCHFCTRLA, value);
}

/**
 * @brief Enable or disable the internal HF oscillator in standby sleep.
 *
 * Controls the RUNSTDBY bit in OSCHFCTRLA. Performed via the protected-write
 * sequence.
 *
 * @param enable true to keep the oscillator running in standby, false otherwise.
 */
static inline void clkOscHFRunStandby(bool enable)
{
	uint8_t value = enable ? (CLKCTRL.OSCHFCTRLA | CLKCTRL_RUNSTDBY_bm)
	                       : (CLKCTRL.OSCHFCTRLA & ~CLKCTRL_RUNSTDBY_bm);
	ccp_write_io((void *)&CLKCTRL.OSCHFCTRLA, value);
}

// Internal 32.768 kHz oscillator ---------------------------------------------
/**
 * @brief Enable or disable the internal 32.768 kHz oscillator in standby sleep.
 *
 * Controls the RUNSTDBY bit in OSC32KCTRLA. Performed via the protected-write
 * sequence.
 *
 * @param enable true to keep the oscillator running in standby, false otherwise.
 */
static inline void clkOsc32kRunStandby(bool enable)
{
	uint8_t value = enable ? (CLKCTRL.OSC32KCTRLA | CLKCTRL_RUNSTDBY_bm)
	                       : (CLKCTRL.OSC32KCTRLA & ~CLKCTRL_RUNSTDBY_bm);
	ccp_write_io((void *)&CLKCTRL.OSC32KCTRLA, value);
}

// External 32.768 kHz crystal oscillator -------------------------------------
/**
 * @brief Enable or disable the external 32.768 kHz crystal oscillator.
 *
 * Controls the ENABLE bit in XOSC32KCTRLA. Configure the source and start-up
 * time before enabling. Performed via the protected-write sequence.
 *
 * @param enable true to enable the crystal oscillator, false to disable it.
 */
static inline void clkXosc32kEnable(bool enable)
{
	uint8_t value = enable ? (CLKCTRL.XOSC32KCTRLA | CLKCTRL_ENABLE_bm)
	                       : (CLKCTRL.XOSC32KCTRLA & ~CLKCTRL_ENABLE_bm);
	ccp_write_io((void *)&CLKCTRL.XOSC32KCTRLA, value);
}

/**
 * @brief Select the external 32.768 kHz source type.
 *
 * Controls the SEL bit in XOSC32KCTRLA: clear selects an external crystal on
 * the TOSC pins, set selects an external clock on TOSC1. Change only while the
 * oscillator is disabled. Performed via the protected-write sequence.
 *
 * @param externalClock true for an external clock signal, false for a crystal.
 */
static inline void clkXosc32kExternalClock(bool externalClock)
{
	uint8_t value = externalClock ? (CLKCTRL.XOSC32KCTRLA | CLKCTRL_SEL_bm)
	                              : (CLKCTRL.XOSC32KCTRLA & ~CLKCTRL_SEL_bm);
	ccp_write_io((void *)&CLKCTRL.XOSC32KCTRLA, value);
}

// PLL ------------------------------------------------------------------------
/**
 * @brief Configure the PLL multiplication factor.
 *
 * Writes the MULFAC field of PLLCTRLA. `CLKCTRL_MULFAC_DISABLE_gc` turns the PLL
 * off. The source and run-standby bits are preserved. Performed via the
 * protected-write sequence.
 *
 * @param factor PLL multiplication group code (`CLKCTRL_MULFAC_*_gc`).
 */
static inline void clkPllSetMultiplier(CLKCTRL_MULFAC_t factor)
{
	ccp_write_io((void *)&CLKCTRL.PLLCTRLA,
	             (CLKCTRL.PLLCTRLA & ~CLKCTRL_MULFAC_gm) | factor);
}

/**
 * @brief Enable or disable PLL operation in standby sleep.
 *
 * Controls the RUNSTDBY bit in PLLCTRLA. Performed via the protected-write
 * sequence.
 *
 * @param enable true to keep the PLL running in standby, false otherwise.
 */
static inline void clkPllRunStandby(bool enable)
{
	uint8_t value = enable ? (CLKCTRL.PLLCTRLA | CLKCTRL_RUNSTDBY_bm)
	                       : (CLKCTRL.PLLCTRLA & ~CLKCTRL_RUNSTDBY_bm);
	ccp_write_io((void *)&CLKCTRL.PLLCTRLA, value);
}

/** @} */ // end of clk_driver

#endif /* CLK_H_ */
