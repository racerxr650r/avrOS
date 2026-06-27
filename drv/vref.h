/**
 * @file vref.h
 * @brief VREF driver — inline accessors for the AVR-Dx voltage reference.
 *
 * Provides a thin, zero-overhead set of `static inline` functions that
 * manipulate the registers of the Voltage Reference (VREF) peripheral. The
 * AVR-Dx has a single VREF block, so these functions operate directly on the
 * global `VREF` register set rather than on a caller-supplied pointer.
 *
 * VREF supplies an independent reference to each of three analog peripherals —
 * the ADC, the DAC, and the analog comparator (AC) — selected with
 * `vrefPeripheral_t`. For each, the driver selects the reference source/voltage
 * (`VREF_REFSEL_*_gc`) and optionally forces the reference always-on to remove
 * its start-up settling delay.
 *
 * @note The DAC driver (drv/dac.h) wraps the DAC reference selection in
 *       `dacSetReference()`; that is equivalent to
 *       `vrefSetReference(VREF_DAC0, ...)`.
 *
 * ### Interrupt safety
 *
 * All VREF registers are 8-bit. `vrefSetReference()` and `vrefAlwaysOn()` are
 * read-modify-write (each updates one field while preserving the other), so if a
 * given reference register is reconfigured from both main-line code and an ISR,
 * serialize the access with an `ATOMIC_BLOCK` (see `<util/atomic.h>`). VREF is
 * normally configured once at start-up, before interrupts are enabled, where no
 * masking is needed. There is no byte-pair (`TEMP`) hazard.
 *
 * The bit-mask (`_bm`) and group-code (`_gc`) symbols referenced here are
 * supplied by `<avr/io.h>` for the selected device.
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

#ifndef VREF_H_
#define VREF_H_

/** @addtogroup vref_driver
 * @{
 */

// Includes -------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>

// Data Types -----------------------------------------------------------------
/**
 * @brief Analog peripheral whose reference is being configured.
 *
 * Each selects one of the independent VREF reference registers.
 */
typedef enum
{
	VREF_ADC0,	///< ADC0 reference (ADC0REF)
	VREF_DAC0,	///< DAC0 reference (DAC0REF)
	VREF_AC		///< Analog comparator reference (ACREF)
} vrefPeripheral_t;

// Internal Functions ---------------------------------------------------------
/**
 * @brief Resolve a peripheral selector to its VREF reference register.
 *
 * @param peripheral Target analog peripheral (`vrefPeripheral_t`).
 * @return Pointer to the corresponding reference register.
 */
static inline volatile register8_t *vrefRegister(vrefPeripheral_t peripheral)
{
	switch(peripheral)
	{
		case VREF_DAC0:
			return &VREF.DAC0REF;
		case VREF_AC:
			return &VREF.ACREF;
		case VREF_ADC0:
		default:
			return &VREF.ADC0REF;
	}
}

// Inline Functions -----------------------------------------------------------
/**
 * @brief Select the reference source/voltage for an analog peripheral.
 *
 * Writes the REFSEL field of the peripheral's reference register, choosing an
 * internal bandgap voltage, VDD, or the external VREFA pin. The ALWAYSON bit is
 * preserved.
 *
 * @param peripheral Target analog peripheral (`vrefPeripheral_t`).
 * @param refsel     Reference selection group code (`VREF_REFSEL_*_gc`).
 */
static inline void vrefSetReference(vrefPeripheral_t peripheral, VREF_REFSEL_t refsel)
{
	volatile register8_t *reg = vrefRegister(peripheral);
	*reg = (*reg & ~VREF_REFSEL_gm) | refsel;
}

/**
 * @brief Read the reference source/voltage of an analog peripheral.
 *
 * Returns the REFSEL field of the peripheral's reference register.
 *
 * @param peripheral Target analog peripheral (`vrefPeripheral_t`).
 * @return The current reference selection group code (`VREF_REFSEL_t`).
 */
static inline VREF_REFSEL_t vrefGetReference(vrefPeripheral_t peripheral)
{
	return (VREF_REFSEL_t)(*vrefRegister(peripheral) & VREF_REFSEL_gm);
}

/**
 * @brief Force a peripheral's reference always on, or release it.
 *
 * Controls the ALWAYSON bit of the peripheral's reference register. When set,
 * the reference stays powered so the analog peripheral incurs no reference
 * start-up delay; when cleared, the reference powers down when unused, saving
 * power. The REFSEL field is preserved.
 *
 * @param peripheral Target analog peripheral (`vrefPeripheral_t`).
 * @param enable     true to keep the reference always on, false to allow it to
 *                   power down when unused.
 */
static inline void vrefAlwaysOn(vrefPeripheral_t peripheral, bool enable)
{
	volatile register8_t *reg = vrefRegister(peripheral);
	if(enable)
		*reg |= VREF_ALWAYSON_bm;
	else
		*reg &= ~VREF_ALWAYSON_bm;
}

/** @} */ // end of vref_driver

#endif /* VREF_H_ */
