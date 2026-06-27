/**
 * @file adc.h
 * @brief ADC driver — inline accessors for the AVR-Dx analog-to-digital converter.
 *
 * Provides a thin, zero-overhead set of `static inline` functions that
 * manipulate the registers of the Analog-to-Digital Converter (ADC0). The
 * AVR-Dx has a single ADC, so these functions operate directly on the global
 * `ADC0` register set rather than on a caller-supplied pointer.
 *
 * The driver covers enable/free-run/standby control, resolution and result
 * justification, single-ended vs differential mode, the clock prescaler,
 * sample accumulation, sampling/initialization timing, the positive/negative
 * input multiplexers, conversion start/stop, the start event input, the window
 * comparator, interrupts, the result register, and debug behavior. The
 * reference voltage is selected through the VREF driver.
 *
 * @note The ADC reference is configured with the VREF driver; `adcSetReference()`
 *       is a convenience wrapper around `vrefSetReference(VREF_ADC0, ...)`.
 *
 * ### Interrupt safety
 *
 * The 8-bit control accessors are read-modify-write; serialize with an
 * `ATOMIC_BLOCK` (see `<util/atomic.h>`) if the ADC is reconfigured from both
 * main-line code and an ISR. The 16-bit `RES`, `WINLT`, and `WINHT` registers
 * are accessed as a low/high byte pair via the ADC's `TEMP` register; an
 * interrupt that performs its own 16-bit ADC access between the two bytes
 * corrupts the result, so `adcGetResultAtomic()` is provided for reading `RES`
 * concurrently. The interrupt-flag clear is write-one-to-clear and not
 * read-modify-write.
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

#ifndef ADC_H_
#define ADC_H_

/** @addtogroup adc_driver
 * @{
 */

// Includes -------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <util/atomic.h>
#include "vref.h"

// Data Types -----------------------------------------------------------------
/**
 * @brief ADC interrupt/flag source selector.
 *
 * Bit mask identifying which ADC interrupt sources to act on. Values may be
 * OR'd together. The bit positions match the INTCTRL and INTFLAGS registers, so
 * the same mask applies to both.
 */
typedef enum
{
	ADC_INT_RESRDY = ADC_RESRDY_bm,	///< Result-ready interrupt
	ADC_INT_WCMP   = ADC_WCMP_bm,	///< Window-comparator interrupt
	ADC_INT_ALL    = ADC_RESRDY_bm | ADC_WCMP_bm	///< Both interrupt sources
} adcInt_t;

// Inline Functions -----------------------------------------------------------
// Run control ----------------------------------------------------------------
/**
 * @brief Enable the ADC.
 *
 * Sets the ENABLE bit in CTRLA. Configure the reference, prescaler, and inputs
 * before enabling.
 */
static inline void adcEnable(void)
{
	ADC0.CTRLA |= ADC_ENABLE_bm;
}

/**
 * @brief Disable the ADC.
 *
 * Clears the ENABLE bit in CTRLA.
 */
static inline void adcDisable(void)
{
	ADC0.CTRLA &= ~ADC_ENABLE_bm;
}

/**
 * @brief Enable or disable free-running conversion mode.
 *
 * Controls the FREERUN bit in CTRLA. In free-running mode the ADC starts a new
 * conversion automatically after each result.
 *
 * @param enable true for free-running, false for single conversions.
 */
static inline void adcFreeRun(bool enable)
{
	if(enable)
		ADC0.CTRLA |= ADC_FREERUN_bm;
	else
		ADC0.CTRLA &= ~ADC_FREERUN_bm;
}

/**
 * @brief Select the conversion resolution.
 *
 * Writes the RESSEL field of CTRLA (12-bit or 10-bit). Other CTRLA bits are
 * preserved.
 *
 * @param resolution Resolution group code (`ADC_RESSEL_*_gc`).
 */
static inline void adcSetResolution(ADC_RESSEL_t resolution)
{
	ADC0.CTRLA = (ADC0.CTRLA & ~ADC_RESSEL_gm) | resolution;
}

/**
 * @brief Select left- or right-adjusted results.
 *
 * Controls the LEFTADJ bit in CTRLA.
 *
 * @param leftAdjust true for left-adjusted, false for right-adjusted results.
 */
static inline void adcLeftAdjust(bool leftAdjust)
{
	if(leftAdjust)
		ADC0.CTRLA |= ADC_LEFTADJ_bm;
	else
		ADC0.CTRLA &= ~ADC_LEFTADJ_bm;
}

/**
 * @brief Select single-ended or differential conversion mode.
 *
 * Writes the CONVMODE bit in CTRLA.
 *
 * @param mode Conversion-mode group code (`ADC_CONVMODE_*_gc`).
 */
static inline void adcSetConversionMode(ADC_CONVMODE_t mode)
{
	ADC0.CTRLA = (ADC0.CTRLA & ~ADC_CONVMODE_bm) | mode;
}

/**
 * @brief Enable or disable ADC operation while the CPU is in standby sleep.
 *
 * Controls the RUNSTBY bit in CTRLA.
 *
 * @param enable true to keep the ADC running in standby, false otherwise.
 */
static inline void adcRunStandby(bool enable)
{
	if(enable)
		ADC0.CTRLA |= ADC_RUNSTBY_bm;
	else
		ADC0.CTRLA &= ~ADC_RUNSTBY_bm;
}

// Reference (delegated to the VREF driver) -----------------------------------
/**
 * @brief Select the ADC voltage reference.
 *
 * Sets the full-scale conversion reference. A convenience wrapper around
 * `vrefSetReference(VREF_ADC0, ref)` from the VREF driver.
 *
 * @param ref Voltage reference selector (`VREF_REFSEL_*_gc`).
 */
static inline void adcSetReference(VREF_REFSEL_t ref)
{
	vrefSetReference(VREF_ADC0, ref);
}

// Timing and accumulation ----------------------------------------------------
/**
 * @brief Select the number of accumulated samples per result.
 *
 * Writes the SAMPNUM field of CTRLB. Accumulating samples improves resolution
 * and noise rejection; the sum appears in the result register.
 *
 * @param samples Accumulation group code (`ADC_SAMPNUM_*_gc`).
 */
static inline void adcSetAccumulation(ADC_SAMPNUM_t samples)
{
	ADC0.CTRLB = samples;
}

/**
 * @brief Select the ADC clock prescaler.
 *
 * Writes the PRESC field of CTRLC, dividing CLK_PER down to the ADC clock.
 *
 * @param prescaler Prescaler group code (`ADC_PRESC_*_gc`).
 */
static inline void adcSetPrescaler(ADC_PRESC_t prescaler)
{
	ADC0.CTRLC = (ADC0.CTRLC & ~ADC_PRESC_gm) | prescaler;
}

/**
 * @brief Select the initialization (start-up) delay.
 *
 * Writes the INITDLY field of CTRLD, the delay applied before the first
 * conversion after the ADC or reference is enabled. Other CTRLD bits are
 * preserved.
 *
 * @param delay Initialization-delay group code (`ADC_INITDLY_*_gc`).
 */
static inline void adcSetInitDelay(ADC_INITDLY_t delay)
{
	ADC0.CTRLD = (ADC0.CTRLD & ~ADC_INITDLY_gm) | delay;
}

/**
 * @brief Select the inter-sample delay used during accumulation.
 *
 * Writes the SAMPDLY field of CTRLD. Other CTRLD bits are preserved.
 *
 * @param delay Sample-delay group code (`ADC_SAMPDLY_*_gc`).
 */
static inline void adcSetSampleDelay(ADC_SAMPDLY_t delay)
{
	ADC0.CTRLD = (ADC0.CTRLD & ~ADC_SAMPDLY_gm) | delay;
}

/**
 * @brief Set the sample length (sampling-capacitor charge time).
 *
 * Writes the SAMPLEN field of SAMPCTRL, extending the sampling time in ADC
 * clock cycles for high-impedance sources.
 *
 * @param length Additional sampling cycles (0–127).
 */
static inline void adcSetSampleLength(uint8_t length)
{
	ADC0.SAMPCTRL = length;
}

// Input multiplexers ---------------------------------------------------------
/**
 * @brief Select the positive ADC input.
 *
 * Writes the MUXPOS register (an analog pin or internal source).
 *
 * @param mux Positive-input group code (`ADC_MUXPOS_*_gc`).
 */
static inline void adcSetPositiveInput(ADC_MUXPOS_t mux)
{
	ADC0.MUXPOS = mux;
}

/**
 * @brief Select the negative ADC input.
 *
 * Writes the MUXNEG register, used as the negative input in differential mode.
 *
 * @param mux Negative-input group code (`ADC_MUXNEG_*_gc`).
 */
static inline void adcSetNegativeInput(ADC_MUXNEG_t mux)
{
	ADC0.MUXNEG = mux;
}

// Conversion control ---------------------------------------------------------
/**
 * @brief Start a conversion.
 *
 * Sets the STCONV bit in COMMAND. In free-running mode this starts the
 * continuous sequence; otherwise it starts a single conversion.
 */
static inline void adcStartConversion(void)
{
	ADC0.COMMAND = ADC_STCONV_bm;
}

/**
 * @brief Stop an ongoing conversion.
 *
 * Sets the SPCONV bit in COMMAND, halting free-running conversions.
 */
static inline void adcStopConversion(void)
{
	ADC0.COMMAND = ADC_SPCONV_bm;
}

/**
 * @brief Enable or disable starting conversions from an event.
 *
 * Controls the STARTEI bit in EVCTRL, allowing an event-system channel to
 * trigger conversions.
 *
 * @param enable true to enable the start event input, false to disable it.
 */
static inline void adcEnableStartEvent(bool enable)
{
	if(enable)
		ADC0.EVCTRL |= ADC_STARTEI_bm;
	else
		ADC0.EVCTRL &= ~ADC_STARTEI_bm;
}

// Window comparator ----------------------------------------------------------
/**
 * @brief Select the window-comparator mode.
 *
 * Writes the WINCM field of CTRLE, which raises the window interrupt when the
 * result is below, above, inside, or outside the configured thresholds.
 *
 * @param mode Window-comparator mode group code (`ADC_WINCM_*_gc`).
 */
static inline void adcSetWindowMode(ADC_WINCM_t mode)
{
	ADC0.CTRLE = mode;
}

/**
 * @brief Set the window-comparator low threshold.
 *
 * Writes the 16-bit WINLT register.
 *
 * @param threshold Low threshold value.
 */
static inline void adcSetWindowLow(uint16_t threshold)
{
	ADC0.WINLT = threshold;
}

/**
 * @brief Set the window-comparator high threshold.
 *
 * Writes the 16-bit WINHT register.
 *
 * @param threshold High threshold value.
 */
static inline void adcSetWindowHigh(uint16_t threshold)
{
	ADC0.WINHT = threshold;
}

// Interrupt control ----------------------------------------------------------
/**
 * @brief Enable one or more ADC interrupt sources.
 *
 * Sets the selected source bits in INTCTRL without disturbing other enabled
 * sources.
 *
 * @param mask Interrupt source(s) to enable (`adcInt_t`, may be OR'd).
 */
static inline void adcEnableInterrupt(adcInt_t mask)
{
	ADC0.INTCTRL |= mask;
}

/**
 * @brief Disable one or more ADC interrupt sources.
 *
 * Clears the selected source bits in INTCTRL.
 *
 * @param mask Interrupt source(s) to disable (`adcInt_t`, may be OR'd).
 */
static inline void adcDisableInterrupt(adcInt_t mask)
{
	ADC0.INTCTRL &= ~mask;
}

/**
 * @brief Read the pending ADC interrupt flags.
 *
 * Returns INTFLAGS masked to the valid interrupt sources.
 *
 * @return Bit mask of pending interrupt flags (`adcInt_t`).
 */
static inline adcInt_t adcGetInterruptFlags(void)
{
	return (adcInt_t)(ADC0.INTFLAGS & ADC_INT_ALL);
}

/**
 * @brief Clear one or more pending ADC interrupt flags.
 *
 * Flags are cleared by writing a one to them; only the requested bits are
 * written so unrelated pending flags are preserved.
 *
 * @param mask Interrupt flag(s) to clear (`adcInt_t`, may be OR'd).
 */
static inline void adcClearInterruptFlags(adcInt_t mask)
{
	ADC0.INTFLAGS = mask;
}

/**
 * @brief Report whether a conversion result is ready.
 *
 * Reads the RESRDY bit of INTFLAGS. This flag is cleared by reading the result
 * register or by writing a one to it.
 *
 * @return true if a result is ready, false otherwise.
 */
static inline bool adcResultReady(void)
{
	return (ADC0.INTFLAGS & ADC_RESRDY_bm) != 0;
}

// Result ---------------------------------------------------------------------
/**
 * @brief Read the 16-bit conversion result (RES).
 *
 * Returns the accumulator result register. Reading it also clears the
 * result-ready flag.
 *
 * @return The conversion result.
 */
static inline uint16_t adcGetResult(void)
{
	return ADC0.RES;
}

/**
 * @brief Atomically read the 16-bit conversion result (RES).
 *
 * Reads RES with interrupts disabled so the low/high byte pair (and the shared
 * `TEMP` register) cannot be corrupted by an ISR that also performs a 16-bit
 * ADC access. The previous global interrupt state is restored on exit. Use this
 * when the result is read from both main-line code and an ISR; otherwise
 * `adcGetResult()` is sufficient and cheaper.
 *
 * @return The conversion result.
 */
static inline uint16_t adcGetResultAtomic(void)
{
	uint16_t result;
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		result = ADC0.RES;
	}
	return result;
}

// Debug control --------------------------------------------------------------
/**
 * @brief Enable or disable ADC operation while halted in debug.
 *
 * Controls the DBGRUN bit in DBGCTRL.
 *
 * @param enable true to keep the ADC running while the debugger has the CPU
 *               halted, false to freeze it.
 */
static inline void adcDebugRun(bool enable)
{
	if(enable)
		ADC0.DBGCTRL |= ADC_DBGRUN_bm;
	else
		ADC0.DBGCTRL &= ~ADC_DBGRUN_bm;
}

/** @} */ // end of adc_driver

#endif /* ADC_H_ */
