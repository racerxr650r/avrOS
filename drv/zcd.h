/**
 * @file zcd.h
 * @brief ZCD driver — inline accessors for the AVR-Dx zero-cross detector.
 *
 * Provides a thin, zero-overhead set of `static inline` functions that
 * manipulate the registers of a Zero-Cross Detector (ZCD). Each function
 * operates on a caller-supplied `ZCD_t *` so the same driver serves any ZCD
 * instance (`&ZCD0`, ...).
 *
 * The ZCD detects when an AC voltage on its input pin crosses the zero-volt
 * threshold and produces an output/event and optional interrupt. The driver
 * enables the detector, configures inversion, the output pad, and standby
 * behavior, selects the interrupt edge, and reads the comparator state and flag.
 *
 * ### Interrupt safety
 *
 * All ZCD registers are 8-bit. The CTRLA accessors are read-modify-write; if a
 * ZCD is reconfigured from both main-line code and an ISR, serialize with an
 * `ATOMIC_BLOCK` (see `<util/atomic.h>`). The flag-clear writes one bit to the
 * write-one-to-clear STATUS register and is not read-modify-write. There is no
 * byte-pair (`TEMP`) hazard.
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

#ifndef ZCD_H_
#define ZCD_H_

/** @addtogroup zcd_driver
 * @{
 */

// Includes -------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>

// Inline Functions -----------------------------------------------------------
// Run control ----------------------------------------------------------------
/**
 * @brief Enable the zero-cross detector.
 *
 * Sets the ENABLE bit in CTRLA.
 *
 * @param zcd Pointer to the ZCD peripheral.
 */
static inline void zcdEnable(ZCD_t *zcd)
{
	zcd->CTRLA |= ZCD_ENABLE_bm;
}

/**
 * @brief Disable the zero-cross detector.
 *
 * Clears the ENABLE bit in CTRLA.
 *
 * @param zcd Pointer to the ZCD peripheral.
 */
static inline void zcdDisable(ZCD_t *zcd)
{
	zcd->CTRLA &= ~ZCD_ENABLE_bm;
}

/**
 * @brief Invert or un-invert the ZCD input signal.
 *
 * Controls the INVERT bit in CTRLA, swapping the polarity of the detected
 * crossing.
 *
 * @param zcd    Pointer to the ZCD peripheral.
 * @param invert true to invert the input, false for normal polarity.
 */
static inline void zcdInvert(ZCD_t *zcd, bool invert)
{
	if(invert)
		zcd->CTRLA |= ZCD_INVERT_bm;
	else
		zcd->CTRLA &= ~ZCD_INVERT_bm;
}

/**
 * @brief Enable or disable the ZCD output pad.
 *
 * Controls the OUTEN bit in CTRLA, driving the detector output onto its pin.
 *
 * @param zcd    Pointer to the ZCD peripheral.
 * @param enable true to drive the output pin, false to release it.
 */
static inline void zcdOutputEnable(ZCD_t *zcd, bool enable)
{
	if(enable)
		zcd->CTRLA |= ZCD_OUTEN_bm;
	else
		zcd->CTRLA &= ~ZCD_OUTEN_bm;
}

/**
 * @brief Enable or disable ZCD operation while the CPU is in standby sleep.
 *
 * Controls the RUNSTDBY bit in CTRLA.
 *
 * @param zcd    Pointer to the ZCD peripheral.
 * @param enable true to keep the detector running in standby, false otherwise.
 */
static inline void zcdRunStandby(ZCD_t *zcd, bool enable)
{
	if(enable)
		zcd->CTRLA |= ZCD_RUNSTDBY_bm;
	else
		zcd->CTRLA &= ~ZCD_RUNSTDBY_bm;
}

// Interrupt control ----------------------------------------------------------
/**
 * @brief Select the interrupt/event edge.
 *
 * Writes the INTMODE field of INTCTRL, choosing which crossing edge (rising,
 * falling, both, or none) raises the interrupt.
 *
 * @param zcd  Pointer to the ZCD peripheral.
 * @param mode Interrupt edge group code (`ZCD_INTMODE_*_gc`).
 */
static inline void zcdSetInterruptMode(ZCD_t *zcd, ZCD_INTMODE_t mode)
{
	zcd->INTCTRL = (zcd->INTCTRL & ~ZCD_INTMODE_gm) | mode;
}

/**
 * @brief Report whether the zero-cross interrupt flag is set.
 *
 * Reads the CROSSIF bit of STATUS.
 *
 * @param zcd Pointer to the ZCD peripheral.
 * @return true if a crossing is pending, false otherwise.
 */
static inline bool zcdGetInterruptFlag(const ZCD_t *zcd)
{
	return (zcd->STATUS & ZCD_CROSSIF_bm) != 0;
}

/**
 * @brief Clear the pending zero-cross interrupt flag.
 *
 * Writes a one to the CROSSIF bit of STATUS.
 *
 * @param zcd Pointer to the ZCD peripheral.
 */
static inline void zcdClearInterruptFlag(ZCD_t *zcd)
{
	zcd->STATUS = ZCD_CROSSIF_bm;
}

// Status ---------------------------------------------------------------------
/**
 * @brief Read the current detector output state.
 *
 * Reads the STATE bit of STATUS, the instantaneous comparator output.
 *
 * @param zcd Pointer to the ZCD peripheral.
 * @return true if the input is above the zero-cross level, false otherwise.
 */
static inline bool zcdGetState(const ZCD_t *zcd)
{
	return (zcd->STATUS & ZCD_STATE_bm) != 0;
}

/** @} */ // end of zcd_driver

#endif /* ZCD_H_ */
