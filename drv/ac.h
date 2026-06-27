/**
 * @file ac.h
 * @brief AC driver — inline accessors for the AVR-Dx analog comparator.
 *
 * Provides a thin, zero-overhead set of `static inline` functions that
 * manipulate the registers of an Analog Comparator (AC). Each function operates
 * on a caller-supplied `AC_t *` so the same driver serves any AC instance
 * (`&AC0`, `&AC1`, `&AC2`).
 *
 * The comparator compares a positive and a negative analog input and produces a
 * digital output, an optional pin output/event, and an interrupt. The driver
 * covers enable, hysteresis, power profile, output pad and standby control, the
 * positive/negative input multiplexers, the internal DAC reference, output
 * inversion and initial value, the optional window mode, interrupt control, and
 * the comparator and window status.
 *
 * @note The internal DAC reference voltage used as a comparator input
 *       (`AC_MUXNEG_DACREF_gc`) derives from the voltage reference; select that
 *       reference with `acSetReference()`, a wrapper around the VREF driver. The
 *       reference (`VREF.ACREF`) is shared by all analog comparators.
 *
 * ### Interrupt safety
 *
 * All AC registers are 8-bit. The configuration accessors are read-modify-write;
 * if an AC is reconfigured from both main-line code and an ISR, serialize with
 * an `ATOMIC_BLOCK` (see `<util/atomic.h>`). The flag-clear writes one bit to the
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

#ifndef AC_H_
#define AC_H_

/** @addtogroup ac_driver
 * @{
 */

// Includes -------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include "vref.h"

// Inline Functions -----------------------------------------------------------
// Run control ----------------------------------------------------------------
/**
 * @brief Enable the analog comparator.
 *
 * Sets the ENABLE bit in CTRLA. Configure the inputs and reference before
 * enabling.
 *
 * @param ac Pointer to the AC peripheral.
 */
static inline void acEnable(AC_t *ac)
{
	ac->CTRLA |= AC_ENABLE_bm;
}

/**
 * @brief Disable the analog comparator.
 *
 * Clears the ENABLE bit in CTRLA.
 *
 * @param ac Pointer to the AC peripheral.
 */
static inline void acDisable(AC_t *ac)
{
	ac->CTRLA &= ~AC_ENABLE_bm;
}

/**
 * @brief Select the comparator hysteresis.
 *
 * Writes the HYSMODE field of CTRLA, adding input hysteresis to reject noise
 * near the comparison threshold. Other CTRLA bits are preserved.
 *
 * @param ac   Pointer to the AC peripheral.
 * @param mode Hysteresis group code (`AC_HYSMODE_*_gc`).
 */
static inline void acSetHysteresis(AC_t *ac, AC_HYSMODE_t mode)
{
	ac->CTRLA = (ac->CTRLA & ~AC_HYSMODE_gm) | mode;
}

/**
 * @brief Select the comparator power/response-time profile.
 *
 * Writes the POWER field of CTRLA, trading response time against power
 * consumption. Other CTRLA bits are preserved.
 *
 * @param ac      Pointer to the AC peripheral.
 * @param profile Power profile group code (`AC_POWER_*_gc`).
 */
static inline void acSetPowerProfile(AC_t *ac, AC_POWER_t profile)
{
	ac->CTRLA = (ac->CTRLA & ~AC_POWER_gm) | profile;
}

/**
 * @brief Enable or disable the comparator output pad.
 *
 * Controls the OUTEN bit in CTRLA, driving the comparator output onto its pin.
 *
 * @param ac     Pointer to the AC peripheral.
 * @param enable true to drive the output pin, false to release it.
 */
static inline void acOutputEnable(AC_t *ac, bool enable)
{
	if(enable)
		ac->CTRLA |= AC_OUTEN_bm;
	else
		ac->CTRLA &= ~AC_OUTEN_bm;
}

/**
 * @brief Enable or disable AC operation while the CPU is in standby sleep.
 *
 * Controls the RUNSTDBY bit in CTRLA.
 *
 * @param ac     Pointer to the AC peripheral.
 * @param enable true to keep the comparator running in standby, false otherwise.
 */
static inline void acRunStandby(AC_t *ac, bool enable)
{
	if(enable)
		ac->CTRLA |= AC_RUNSTDBY_bm;
	else
		ac->CTRLA &= ~AC_RUNSTDBY_bm;
}

// Window mode ----------------------------------------------------------------
/**
 * @brief Select the window-comparison mode.
 *
 * Writes the WINSEL field of CTRLB, which groups this comparator with adjacent
 * comparators to form a voltage window. `AC_WINSEL_DISABLED_gc` selects normal
 * (single comparator) operation.
 *
 * @param ac     Pointer to the AC peripheral.
 * @param select Window-selection group code (`AC_WINSEL_*_gc`).
 */
static inline void acSetWindowMode(AC_t *ac, AC_WINSEL_t select)
{
	ac->CTRLB = (ac->CTRLB & ~AC_WINSEL_gm) | select;
}

// Input multiplexers ---------------------------------------------------------
/**
 * @brief Select the positive comparator input.
 *
 * Writes the MUXPOS field of MUXCTRL. Other MUXCTRL bits are preserved.
 *
 * @param ac  Pointer to the AC peripheral.
 * @param mux Positive-input group code (`AC_MUXPOS_*_gc`).
 */
static inline void acSetPositiveInput(AC_t *ac, AC_MUXPOS_t mux)
{
	ac->MUXCTRL = (ac->MUXCTRL & ~AC_MUXPOS_gm) | mux;
}

/**
 * @brief Select the negative comparator input.
 *
 * Writes the MUXNEG field of MUXCTRL (a pin or the internal DAC reference).
 * Other MUXCTRL bits are preserved.
 *
 * @param ac  Pointer to the AC peripheral.
 * @param mux Negative-input group code (`AC_MUXNEG_*_gc`).
 */
static inline void acSetNegativeInput(AC_t *ac, AC_MUXNEG_t mux)
{
	ac->MUXCTRL = (ac->MUXCTRL & ~AC_MUXNEG_gm) | mux;
}

/**
 * @brief Invert or un-invert the comparator output.
 *
 * Controls the INVERT bit in MUXCTRL.
 *
 * @param ac     Pointer to the AC peripheral.
 * @param invert true to invert the output, false for normal polarity.
 */
static inline void acInvertOutput(AC_t *ac, bool invert)
{
	if(invert)
		ac->MUXCTRL |= AC_INVERT_bm;
	else
		ac->MUXCTRL &= ~AC_INVERT_bm;
}

// Reference (delegated to the VREF driver) -----------------------------------
/**
 * @brief Select the analog comparator voltage reference.
 *
 * Sets the reference from which the DAC reference level (`acSetDacRef()`) is
 * derived. A convenience wrapper around `vrefSetReference(VREF_AC, ref)` from the
 * VREF driver.
 *
 * @note `VREF.ACREF` is shared by all analog comparators, so this affects every
 *       AC instance — it takes no `AC_t *`.
 *
 * @param ref Voltage reference selector (`VREF_REFSEL_*_gc`).
 */
static inline void acSetReference(VREF_REFSEL_t ref)
{
	vrefSetReference(VREF_AC, ref);
}

// DAC reference --------------------------------------------------------------
/**
 * @brief Set the internal DAC reference level for the comparator.
 *
 * Writes the 8-bit DACREF register, which sets the scaled reference voltage
 * available as a comparator input (selected via `AC_MUXNEG_DACREF_gc`). The
 * level is `VREF * value / 256`, where VREF is the AC reference chosen in the
 * VREF driver.
 *
 * @param ac    Pointer to the AC peripheral.
 * @param value Reference level (0–255).
 */
static inline void acSetDacRef(AC_t *ac, uint8_t value)
{
	ac->DACREF = value;
}

// Interrupt control ----------------------------------------------------------
/**
 * @brief Enable or disable the comparator interrupt.
 *
 * Controls the CMP bit in INTCTRL.
 *
 * @param ac     Pointer to the AC peripheral.
 * @param enable true to enable the interrupt, false to disable it.
 */
static inline void acEnableInterrupt(AC_t *ac, bool enable)
{
	if(enable)
		ac->INTCTRL |= AC_CMP_bm;
	else
		ac->INTCTRL &= ~AC_CMP_bm;
}

/**
 * @brief Select the interrupt trigger mode.
 *
 * Writes the INTMODE field of INTCTRL. In normal mode use the
 * `AC_INTMODE_NORMAL_*_gc` codes (edge selection); in window mode use the
 * `AC_INTMODE_WINDOW_*_gc` codes (window-region selection). Both occupy the same
 * register field.
 *
 * @param ac   Pointer to the AC peripheral.
 * @param mode Interrupt mode group code (`AC_INTMODE_NORMAL_*_gc` or
 *             `AC_INTMODE_WINDOW_*_gc`).
 */
static inline void acSetInterruptMode(AC_t *ac, uint8_t mode)
{
	ac->INTCTRL = (ac->INTCTRL & ~AC_INTMODE_NORMAL_gm) | mode;
}

/**
 * @brief Report whether the comparator interrupt flag is set.
 *
 * Reads the CMPIF bit of STATUS.
 *
 * @param ac Pointer to the AC peripheral.
 * @return true if the interrupt is pending, false otherwise.
 */
static inline bool acGetInterruptFlag(const AC_t *ac)
{
	return (ac->STATUS & AC_CMPIF_bm) != 0;
}

/**
 * @brief Clear the pending comparator interrupt flag.
 *
 * Writes a one to the CMPIF bit of STATUS.
 *
 * @param ac Pointer to the AC peripheral.
 */
static inline void acClearInterruptFlag(AC_t *ac)
{
	ac->STATUS = AC_CMPIF_bm;
}

// Status ---------------------------------------------------------------------
/**
 * @brief Read the current comparator output state.
 *
 * Reads the CMPSTATE bit of STATUS, the instantaneous comparator output.
 *
 * @param ac Pointer to the AC peripheral.
 * @return true if the positive input is above the negative input, false
 *         otherwise.
 */
static inline bool acGetState(const AC_t *ac)
{
	return (ac->STATUS & AC_CMPSTATE_bm) != 0;
}

/**
 * @brief Read the window-comparison state.
 *
 * Returns the WINSTATE field of STATUS, indicating whether the input is above,
 * inside, or below the configured voltage window. Only meaningful in window
 * mode.
 *
 * @param ac Pointer to the AC peripheral.
 * @return Window-state group code (`AC_WINSTATE_t`).
 */
static inline AC_WINSTATE_t acGetWindowState(const AC_t *ac)
{
	return (AC_WINSTATE_t)(ac->STATUS & AC_WINSTATE_gm);
}

/** @} */ // end of ac_driver

#endif /* AC_H_ */
