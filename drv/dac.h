/**
 * @file dac.h
 * @brief DAC driver — inline accessors for the AVR-Dx 10-bit DAC.
 *
 * Provides a thin, zero-overhead set of `static inline` functions that
 * manipulate the registers of the Digital-to-Analog Converter (DAC0) and its
 * voltage reference. The AVR-Dx has a single DAC instance, so these functions
 * operate directly on the global `DAC0` register set (and the `VREF`
 * peripheral) rather than on a caller-supplied pointer.
 *
 * The functions cover reference selection, output-buffer and standby control,
 * enable/disable, and writing the conversion data register. Application-level
 * concerns — sample scaling, signed-to-unsigned offset, clamping, and
 * configuring the digital input buffer on the DAC output pin — are left to the
 * caller (see srv/pcm.c).
 *
 * ### Output data format
 *
 * The `DATA` register is **left-justified**: the 10-bit conversion value
 * occupies bits [15:6] (`DAC_DATA_gm`), so a raw 0–`DAC_MAX` code must be
 * shifted left by `DAC_DATA_gp` before being written. `dacSetData()` writes the
 * `DATA` register verbatim and does not shift; callers that work in 10-bit
 * codes are responsible for the justification.
 *
 * ### Interrupt safety
 *
 * These accessors are **not** interrupt-safe. The `CTRLA` read-modify-write
 * functions (`dacEnable()`, `dacDisable()`, `dacOutputBufferEnable()`,
 * `dacRunStandby()`) can lose a concurrent ISR update to `CTRLA`. Unlike the
 * 16-bit timer/RTC registers, the DAC has no shared `TEMP` register, so writing
 * the 16-bit `DATA` register carries no byte-pair corruption hazard; it is
 * still two instructions, however, so if `DATA` is written from both main-line
 * code and an ISR, wrap the affected accesses in an `ATOMIC_BLOCK` (see
 * `<util/atomic.h>`). The typical use writes `DATA` from a single context.
 *
 * The group-code (`_gc`), bit-mask (`_bm`), and bit-position (`_bp`) symbols
 * referenced here are supplied by `<avr/io.h>` for the selected device.
 *
 * Created: 7/11/2021 11:57:01 PM
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

#ifndef DAC_H_
#define DAC_H_

/** @addtogroup dac_driver
 * @{
 */

// Includes -------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include "vref.h"

// Constants ------------------------------------------------------------------
#define DAC_MAX		0x03ff	///< Maximum 10-bit DAC code (full scale).
#define DAC_MID		0x01ff	///< Mid-scale 10-bit DAC code (~half VREF).
#define DAC_MIN		0x0000	///< Minimum 10-bit DAC code (zero scale).

// Inline Functions -----------------------------------------------------------
// Reference and pin control --------------------------------------------------
/**
 * @brief Select the DAC voltage reference.
 *
 * Sets the full-scale output reference for the DAC. A convenience wrapper around
 * `vrefSetReference(VREF_DAC0, vRef)` from the VREF driver.
 *
 * @param vRef Voltage reference selector (`VREF_REFSEL_*_gc`).
 */
static inline void dacSetReference(VREF_REFSEL_t vRef)
{
	vrefSetReference(VREF_DAC0, vRef);
}

/**
 * @brief Enable or disable the DAC analog output buffer.
 *
 * Controls the OUTEN bit in CTRLA, which connects the DAC output to its
 * dedicated pin. Other CTRLA bits — including the enable bit — are preserved.
 *
 * @param enable true to drive the output pin, false to release it.
 */
static inline void dacOutputBufferEnable(bool enable)
{
	if(enable)
		DAC0.CTRLA |= DAC_OUTEN_bm;
	else
		DAC0.CTRLA &= ~DAC_OUTEN_bm;
}

// Run control ----------------------------------------------------------------
/**
 * @brief Enable the DAC.
 *
 * Sets the ENABLE bit in CTRLA. Select the reference and load an initial data
 * value before enabling.
 */
static inline void dacEnable(void)
{
	DAC0.CTRLA |= DAC_ENABLE_bm;
}

/**
 * @brief Disable the DAC.
 *
 * Clears the ENABLE bit in CTRLA.
 */
static inline void dacDisable(void)
{
	DAC0.CTRLA &= ~DAC_ENABLE_bm;
}

/**
 * @brief Report whether the DAC is enabled.
 *
 * Reads the ENABLE bit of CTRLA.
 *
 * @return true if the DAC is enabled, false otherwise.
 */
static inline bool dacIsEnabled(void)
{
	return (DAC0.CTRLA & DAC_ENABLE_bm) != 0;
}

/**
 * @brief Enable or disable DAC operation while the CPU is in standby sleep.
 *
 * Controls the RUNSTDBY bit in CTRLA.
 *
 * @param enable true to keep the DAC running in standby, false otherwise.
 */
static inline void dacRunStandby(bool enable)
{
	if(enable)
		DAC0.CTRLA |= DAC_RUNSTDBY_bm;
	else
		DAC0.CTRLA &= ~DAC_RUNSTDBY_bm;
}

// Data output ----------------------------------------------------------------
/**
 * @brief Write the DAC conversion data register (DATA).
 *
 * Writes the 16-bit DATA register verbatim. The DAC output updates to the new
 * value on the write. Note the register is left-justified: the 10-bit code
 * occupies bits [15:6], so callers working with a 0–`DAC_MAX` code must shift
 * the value left by `DAC_DATA_gp` before passing it here.
 *
 * @param data Raw value to write to DATA (left-justified conversion code).
 */
static inline void dacSetData(uint16_t data)
{
	DAC0.DATA = data;
}

/** @} */ // end of dac_driver

#endif /* DAC_H_ */
