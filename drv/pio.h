/**
 * @file pio.h
 * @brief PIO driver — inline accessors for the AVR-Dx I/O ports (PORT).
 *
 * Provides a thin, zero-overhead set of `static inline` functions that
 * manipulate the registers of an I/O port (PORT). Each function operates on a
 * caller-supplied `PORT_t *` so the same driver serves any port (`&PORTA`,
 * `&PORTB`, ...).
 *
 * The driver controls pin direction, output value, and input reading using the
 * port's dedicated set/clear/toggle registers (which are inherently atomic), the
 * pin-change interrupt flags, the port slew-rate option, and the per-pin
 * configuration (input/sense mode, pull-up, inverted I/O).
 *
 * @note This is the low-level PORT register driver. The GPIO driver (drv/gpio.h)
 *       provides higher-level named GPIO instances with interrupt-driven event
 *       callbacks built on top of the same hardware.
 *
 * Bulk direction/output/input/flag operations take a bit mask (`pioPin_t`, OR
 * pins together). Per-pin configuration takes a pin index (0–7).
 *
 * ### Interrupt safety
 *
 * The direction and output set/clear/toggle registers (`DIRSET`, `OUTSET`, ...)
 * are write-only strobes and are inherently atomic — no masking needed. The
 * per-pin configuration field setters and the slew-rate setter are 8-bit
 * read-modify-write; serialize with an `ATOMIC_BLOCK` (see `<util/atomic.h>`) if
 * used from both main-line code and an ISR. The interrupt-flag clear is
 * write-one-to-clear. All registers are 8-bit; there is no byte-pair (`TEMP`)
 * hazard.
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

#ifndef PIO_H_
#define PIO_H_

/** @addtogroup pio_driver
 * @{
 */

// Includes -------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>

// Data Types -----------------------------------------------------------------
/**
 * @brief Pin bit-mask selector.
 *
 * Bit mask identifying one or more pins within a port for the direction,
 * output, input, and interrupt-flag operations. OR values together to act on
 * several pins at once.
 */
typedef enum
{
	PIO_PIN0 = 0x01,	///< Pin 0
	PIO_PIN1 = 0x02,	///< Pin 1
	PIO_PIN2 = 0x04,	///< Pin 2
	PIO_PIN3 = 0x08,	///< Pin 3
	PIO_PIN4 = 0x10,	///< Pin 4
	PIO_PIN5 = 0x20,	///< Pin 5
	PIO_PIN6 = 0x40,	///< Pin 6
	PIO_PIN7 = 0x80,	///< Pin 7
	PIO_PIN_ALL = 0xFF	///< All pins
} pioPin_t;

// Inline Functions -----------------------------------------------------------
// Direction ------------------------------------------------------------------
/**
 * @brief Configure the selected pins as outputs.
 *
 * Writes the pin mask to DIRSET (atomic set).
 *
 * @param port Pointer to the port.
 * @param pins Bit mask of pins to make outputs (`pioPin_t`).
 */
static inline void pioSetOutput(PORT_t *port, uint8_t pins)
{
	port->DIRSET = pins;
}

/**
 * @brief Configure the selected pins as inputs.
 *
 * Writes the pin mask to DIRCLR (atomic clear).
 *
 * @param port Pointer to the port.
 * @param pins Bit mask of pins to make inputs (`pioPin_t`).
 */
static inline void pioSetInput(PORT_t *port, uint8_t pins)
{
	port->DIRCLR = pins;
}

/**
 * @brief Toggle the direction of the selected pins.
 *
 * Writes the pin mask to DIRTGL (atomic toggle).
 *
 * @param port Pointer to the port.
 * @param pins Bit mask of pins to toggle (`pioPin_t`).
 */
static inline void pioToggleDirection(PORT_t *port, uint8_t pins)
{
	port->DIRTGL = pins;
}

/**
 * @brief Write the whole direction register (DIR).
 *
 * @param port  Pointer to the port.
 * @param value Direction bit mask (1 = output, 0 = input) for all eight pins.
 */
static inline void pioWriteDirection(PORT_t *port, uint8_t value)
{
	port->DIR = value;
}

/**
 * @brief Read the direction register (DIR).
 *
 * @param port Pointer to the port.
 * @return Direction bit mask (1 = output, 0 = input).
 */
static inline uint8_t pioGetDirection(const PORT_t *port)
{
	return port->DIR;
}

// Output ---------------------------------------------------------------------
/**
 * @brief Drive the selected output pins high.
 *
 * Writes the pin mask to OUTSET (atomic set).
 *
 * @param port Pointer to the port.
 * @param pins Bit mask of pins to set (`pioPin_t`).
 */
static inline void pioSet(PORT_t *port, uint8_t pins)
{
	port->OUTSET = pins;
}

/**
 * @brief Drive the selected output pins low.
 *
 * Writes the pin mask to OUTCLR (atomic clear).
 *
 * @param port Pointer to the port.
 * @param pins Bit mask of pins to clear (`pioPin_t`).
 */
static inline void pioClear(PORT_t *port, uint8_t pins)
{
	port->OUTCLR = pins;
}

/**
 * @brief Toggle the selected output pins.
 *
 * Writes the pin mask to OUTTGL (atomic toggle).
 *
 * @param port Pointer to the port.
 * @param pins Bit mask of pins to toggle (`pioPin_t`).
 */
static inline void pioToggle(PORT_t *port, uint8_t pins)
{
	port->OUTTGL = pins;
}

/**
 * @brief Write the whole output register (OUT).
 *
 * @param port  Pointer to the port.
 * @param value Output value bit mask for all eight pins.
 */
static inline void pioWrite(PORT_t *port, uint8_t value)
{
	port->OUT = value;
}

/**
 * @brief Read the output latch register (OUT).
 *
 * Returns the driven output value, not the measured pin level (see
 * `pioRead()`).
 *
 * @param port Pointer to the port.
 * @return The OUT register value.
 */
static inline uint8_t pioReadOutput(const PORT_t *port)
{
	return port->OUT;
}

// Input ----------------------------------------------------------------------
/**
 * @brief Read the input register (IN).
 *
 * Returns the sampled logic level on the port pins.
 *
 * @param port Pointer to the port.
 * @return The IN register value.
 */
static inline uint8_t pioRead(const PORT_t *port)
{
	return port->IN;
}

// Interrupt flags ------------------------------------------------------------
/**
 * @brief Read the pin-change interrupt flags (INTFLAGS).
 *
 * @param port Pointer to the port.
 * @return Bit mask of pins with a pending interrupt flag.
 */
static inline uint8_t pioGetInterruptFlags(const PORT_t *port)
{
	return port->INTFLAGS;
}

/**
 * @brief Clear the selected pin-change interrupt flags.
 *
 * Writes the pin mask to INTFLAGS (write-one-to-clear); only the requested flags
 * are cleared.
 *
 * @param port Pointer to the port.
 * @param pins Bit mask of flags to clear (`pioPin_t`).
 */
static inline void pioClearInterruptFlags(PORT_t *port, uint8_t pins)
{
	port->INTFLAGS = pins;
}

// Port control ---------------------------------------------------------------
/**
 * @brief Enable or disable the port slew-rate limiter.
 *
 * Controls the SRL bit in PORTCTRL, slowing the output edges to reduce EMI.
 *
 * @param port   Pointer to the port.
 * @param enable true to limit the slew rate, false for full-speed edges.
 */
static inline void pioSlewRateLimit(PORT_t *port, bool enable)
{
	if(enable)
		port->PORTCTRL |= PORT_SRL_bm;
	else
		port->PORTCTRL &= ~PORT_SRL_bm;
}

// Per-pin configuration ------------------------------------------------------
/**
 * @brief Write a pin's control register (PINnCTRL).
 *
 * Writes the full configuration byte for one pin: input/sense mode
 * (`PORT_ISC_*_gc`), pull-up (`PORT_PULLUPEN_bm`), and inverted I/O
 * (`PORT_INVEN_bm`), OR'd together.
 *
 * @param port   Pointer to the port.
 * @param pin    Pin index (0–7).
 * @param config Configuration byte to write.
 */
static inline void pioSetPinConfig(PORT_t *port, uint8_t pin, uint8_t config)
{
	(&port->PIN0CTRL)[pin] = config;
}

/**
 * @brief Read a pin's control register (PINnCTRL).
 *
 * @param port Pointer to the port.
 * @param pin  Pin index (0–7).
 * @return The pin's configuration byte.
 */
static inline uint8_t pioGetPinConfig(const PORT_t *port, uint8_t pin)
{
	return (&port->PIN0CTRL)[pin];
}

/**
 * @brief Select a pin's input/sense configuration (PINnCTRL ISC).
 *
 * Writes the ISC field of the pin's control register, choosing the interrupt
 * edge/level sensing or disabling the digital input buffer. Other config bits
 * (pull-up, invert) are preserved.
 *
 * @param port Pointer to the port.
 * @param pin  Pin index (0–7).
 * @param isc  Input/sense group code (`PORT_ISC_*_gc`).
 */
static inline void pioSetInputSense(PORT_t *port, uint8_t pin, PORT_ISC_t isc)
{
	volatile register8_t *reg = &(&port->PIN0CTRL)[pin];
	*reg = (*reg & ~PORT_ISC_gm) | isc;
}

/**
 * @brief Enable or disable a pin's internal pull-up (PINnCTRL PULLUPEN).
 *
 * @param port   Pointer to the port.
 * @param pin    Pin index (0–7).
 * @param enable true to enable the pull-up, false to disable it.
 */
static inline void pioPullup(PORT_t *port, uint8_t pin, bool enable)
{
	volatile register8_t *reg = &(&port->PIN0CTRL)[pin];
	if(enable)
		*reg |= PORT_PULLUPEN_bm;
	else
		*reg &= ~PORT_PULLUPEN_bm;
}

/**
 * @brief Enable or disable a pin's inverted I/O (PINnCTRL INVEN).
 *
 * @param port   Pointer to the port.
 * @param pin    Pin index (0–7).
 * @param invert true to invert the pin's input and output, false for normal.
 */
static inline void pioInvert(PORT_t *port, uint8_t pin, bool invert)
{
	volatile register8_t *reg = &(&port->PIN0CTRL)[pin];
	if(invert)
		*reg |= PORT_INVEN_bm;
	else
		*reg &= ~PORT_INVEN_bm;
}

/**
 * @brief Apply one configuration byte to several pins at once.
 *
 * Loads @p config into PINCONFIG, then writes @p pins to PINCTRLUPD so the full
 * configuration is applied to every selected pin's control register in one
 * operation.
 *
 * @param port   Pointer to the port.
 * @param config Configuration byte (see `pioSetPinConfig()`).
 * @param pins   Bit mask of pins to apply it to (`pioPin_t`).
 */
static inline void pioConfigPins(PORT_t *port, uint8_t config, uint8_t pins)
{
	port->PINCONFIG = config;
	port->PINCTRLUPD = pins;
}

/** @} */ // end of pio_driver

#endif /* PIO_H_ */
