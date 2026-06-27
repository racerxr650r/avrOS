/**
 * @file rst.h
 * @brief Reset driver — inline accessors for the AVR-Dx reset controller.
 *
 * Provides a thin, zero-overhead set of `static inline` functions that
 * manipulate the registers of the Reset Controller (RSTCTRL). The AVR-Dx has a
 * single reset controller, so these functions operate directly on the global
 * `RSTCTRL` register set rather than on a caller-supplied pointer.
 *
 * The driver reads and clears the reset-source flags (which record what caused
 * the most recent reset) and issues a software reset.
 *
 * @note The software-reset register is protected by the Configuration Change
 *       Protection (CCP) mechanism; `rstSoftwareReset()` performs the protected
 *       write via avr-libc's `ccp_write_io()`.
 *
 * ### Interrupt safety
 *
 * The reset flags are typically read and cleared once during start-up. The
 * flags register is write-one-to-clear, so `rstClearFlags()` is a direct write
 * (not read-modify-write) and writes only the requested bits. All registers are
 * 8-bit; there is no byte-pair (`TEMP`) hazard.
 *
 * The bit-mask (`_bm`) symbols referenced here are supplied by `<avr/io.h>` for
 * the selected device.
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

#ifndef RST_H_
#define RST_H_

/** @addtogroup rst_driver
 * @{
 */

// Includes -------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/cpufunc.h>

// Data Types -----------------------------------------------------------------
/**
 * @brief Reset-source flag selector.
 *
 * Bit mask identifying which reset-source flags to test or clear. Values may be
 * OR'd together. The bit positions match the RSTCTRL RSTFR register. After a
 * reset, one or more of these flags indicate the cause; clear them so the next
 * reset's cause can be identified unambiguously.
 */
typedef enum
{
	RST_POWER_ON  = RSTCTRL_PORF_bm,	///< Power-on reset
	RST_BROWNOUT  = RSTCTRL_BORF_bm,	///< Brown-out detector reset
	RST_EXTERNAL  = RSTCTRL_EXTRF_bm,	///< External reset (RESET pin)
	RST_WATCHDOG  = RSTCTRL_WDRF_bm,	///< Watchdog timer reset
	RST_SOFTWARE  = RSTCTRL_SWRF_bm,	///< Software reset
	RST_UPDI      = RSTCTRL_UPDIRF_bm,	///< UPDI (debugger) reset
	RST_ALL       = RSTCTRL_PORF_bm | RSTCTRL_BORF_bm | RSTCTRL_EXTRF_bm |
	                RSTCTRL_WDRF_bm | RSTCTRL_SWRF_bm | RSTCTRL_UPDIRF_bm	///< All sources
} rstFlag_t;

// Inline Functions -----------------------------------------------------------
/**
 * @brief Read the reset-source flags.
 *
 * Returns the RSTFR register: a bit mask of the sources that caused the most
 * recent reset(s). Test against `RST_POWER_ON`, `RST_WATCHDOG`, etc.
 *
 * @return Bit mask of active reset-source flags (`rstFlag_t`).
 */
static inline rstFlag_t rstGetFlags(void)
{
	return (rstFlag_t)RSTCTRL.RSTFR;
}

/**
 * @brief Clear one or more reset-source flags.
 *
 * The RSTFR flags are cleared by writing a one to them; this function writes
 * only the requested bits so unrelated flags are preserved.
 *
 * @param mask Reset-source flag(s) to clear (`rstFlag_t`, may be OR'd).
 */
static inline void rstClearFlags(rstFlag_t mask)
{
	RSTCTRL.RSTFR = mask;
}

/**
 * @brief Issue a software reset.
 *
 * Performs the CCP-protected write that sets the SWRST bit in the software-reset
 * register, triggering an immediate system reset. This function does not return.
 */
static inline void rstSoftwareReset(void)
{
	ccp_write_io((void *)&RSTCTRL.SWRR, RSTCTRL_SWRST_bm);
}

/** @} */ // end of rst_driver

#endif /* RST_H_ */
