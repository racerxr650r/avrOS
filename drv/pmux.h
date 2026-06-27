/**
 * @file pmux.h
 * @brief Port multiplexer driver — inline accessors for the AVR-Dx PORTMUX.
 *
 * Provides a thin, zero-overhead set of `static inline` functions that
 * manipulate the registers of the Port Multiplexer (PORTMUX). The AVR-Dx has a
 * single PORTMUX, so these functions operate directly on the global `PORTMUX`
 * register set rather than on a caller-supplied pointer.
 *
 * PORTMUX selects which physical pins a peripheral's signals are routed to. Each
 * function writes one peripheral signal's routing field, preserving the other
 * fields in the same route register. Configure routing before enabling the
 * peripheral.
 *
 * @note Some signals have only a default routing on a given device; the
 *       corresponding setter then accepts a single-valued group code. The full
 *       set is provided for completeness and portability across the family.
 *
 * ### Interrupt safety
 *
 * All PORTMUX registers are 8-bit and routing is normally configured once at
 * start-up. The setters are read-modify-write; serialize with an `ATOMIC_BLOCK`
 * (see `<util/atomic.h>`) if a route register is modified from both main-line
 * code and an ISR. There is no byte-pair (`TEMP`) hazard.
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

#ifndef PMUX_H_
#define PMUX_H_

/** @addtogroup pmux_driver
 * @{
 */

// Includes -------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>

// Inline Functions -----------------------------------------------------------
// Event system output routing (EVSYSROUTEA) ----------------------------------
/**
 * @brief Route the event-output A (EVOUTA) signal.
 * @param route EVOUTA routing group code (`PORTMUX_EVOUTA_*_gc`).
 */
static inline void pmuxEvOutA(PORTMUX_EVOUTA_t route)
{
	PORTMUX.EVSYSROUTEA = (PORTMUX.EVSYSROUTEA & ~PORTMUX_EVOUTA_bm) | route;
}

/**
 * @brief Route the event-output C (EVOUTC) signal.
 * @param route EVOUTC routing group code (`PORTMUX_EVOUTC_*_gc`).
 */
static inline void pmuxEvOutC(PORTMUX_EVOUTC_t route)
{
	PORTMUX.EVSYSROUTEA = (PORTMUX.EVSYSROUTEA & ~PORTMUX_EVOUTC_bm) | route;
}

/**
 * @brief Route the event-output D (EVOUTD) signal.
 * @param route EVOUTD routing group code (`PORTMUX_EVOUTD_*_gc`).
 */
static inline void pmuxEvOutD(PORTMUX_EVOUTD_t route)
{
	PORTMUX.EVSYSROUTEA = (PORTMUX.EVSYSROUTEA & ~PORTMUX_EVOUTD_bm) | route;
}

// CCL look-up-table routing (CCLROUTEA) --------------------------------------
/**
 * @brief Route the CCL LUT0 output/inputs.
 * @param route LUT0 routing group code (`PORTMUX_LUT0_*_gc`).
 */
static inline void pmuxCclLut0(PORTMUX_LUT0_t route)
{
	PORTMUX.CCLROUTEA = (PORTMUX.CCLROUTEA & ~PORTMUX_LUT0_bm) | route;
}

/**
 * @brief Route the CCL LUT1 output/inputs.
 * @param route LUT1 routing group code (`PORTMUX_LUT1_*_gc`).
 */
static inline void pmuxCclLut1(PORTMUX_LUT1_t route)
{
	PORTMUX.CCLROUTEA = (PORTMUX.CCLROUTEA & ~PORTMUX_LUT1_bm) | route;
}

/**
 * @brief Route the CCL LUT2 output/inputs.
 * @param route LUT2 routing group code (`PORTMUX_LUT2_*_gc`).
 */
static inline void pmuxCclLut2(PORTMUX_LUT2_t route)
{
	PORTMUX.CCLROUTEA = (PORTMUX.CCLROUTEA & ~PORTMUX_LUT2_bm) | route;
}

/**
 * @brief Route the CCL LUT3 output/inputs.
 * @param route LUT3 routing group code (`PORTMUX_LUT3_*_gc`).
 */
static inline void pmuxCclLut3(PORTMUX_LUT3_t route)
{
	PORTMUX.CCLROUTEA = (PORTMUX.CCLROUTEA & ~PORTMUX_LUT3_bm) | route;
}

// USART routing (USARTROUTEA) ------------------------------------------------
/**
 * @brief Route the USART0 signals.
 * @param route USART0 routing group code (`PORTMUX_USART0_*_gc`).
 */
static inline void pmuxUsart0(PORTMUX_USART0_t route)
{
	PORTMUX.USARTROUTEA = (PORTMUX.USARTROUTEA & ~PORTMUX_USART0_gm) | route;
}

/**
 * @brief Route the USART1 signals.
 * @param route USART1 routing group code (`PORTMUX_USART1_*_gc`).
 */
static inline void pmuxUsart1(PORTMUX_USART1_t route)
{
	PORTMUX.USARTROUTEA = (PORTMUX.USARTROUTEA & ~PORTMUX_USART1_gm) | route;
}

/**
 * @brief Route the USART2 signals.
 * @param route USART2 routing group code (`PORTMUX_USART2_*_gc`).
 */
static inline void pmuxUsart2(PORTMUX_USART2_t route)
{
	PORTMUX.USARTROUTEA = (PORTMUX.USARTROUTEA & ~PORTMUX_USART2_gm) | route;
}

// SPI routing (SPIROUTEA) ----------------------------------------------------
/**
 * @brief Route the SPI0 signals.
 * @param route SPI0 routing group code (`PORTMUX_SPI0_*_gc`).
 */
static inline void pmuxSpi0(PORTMUX_SPI0_t route)
{
	PORTMUX.SPIROUTEA = (PORTMUX.SPIROUTEA & ~PORTMUX_SPI0_gm) | route;
}

/**
 * @brief Route the SPI1 signals.
 * @param route SPI1 routing group code (`PORTMUX_SPI1_*_gc`).
 */
static inline void pmuxSpi1(PORTMUX_SPI1_t route)
{
	PORTMUX.SPIROUTEA = (PORTMUX.SPIROUTEA & ~PORTMUX_SPI1_gm) | route;
}

// TWI routing (TWIROUTEA) ----------------------------------------------------
/**
 * @brief Route the TWI0 signals.
 * @param route TWI0 routing group code (`PORTMUX_TWI0_*_gc`).
 */
static inline void pmuxTwi0(PORTMUX_TWI0_t route)
{
	PORTMUX.TWIROUTEA = (PORTMUX.TWIROUTEA & ~PORTMUX_TWI0_gm) | route;
}

// TCA routing (TCAROUTEA) ----------------------------------------------------
/**
 * @brief Route the TCA0 waveform outputs.
 * @param route TCA0 routing group code (`PORTMUX_TCA0_*_gc`).
 */
static inline void pmuxTca0(PORTMUX_TCA0_t route)
{
	PORTMUX.TCAROUTEA = (PORTMUX.TCAROUTEA & ~PORTMUX_TCA0_gm) | route;
}

// TCB routing (TCBROUTEA) ----------------------------------------------------
/**
 * @brief Route the TCB0 waveform output.
 * @param route TCB0 routing group code (`PORTMUX_TCB0_*_gc`).
 */
static inline void pmuxTcb0(PORTMUX_TCB0_t route)
{
	PORTMUX.TCBROUTEA = (PORTMUX.TCBROUTEA & ~PORTMUX_TCB0_bm) | route;
}

/**
 * @brief Route the TCB1 waveform output.
 * @param route TCB1 routing group code (`PORTMUX_TCB1_*_gc`).
 */
static inline void pmuxTcb1(PORTMUX_TCB1_t route)
{
	PORTMUX.TCBROUTEA = (PORTMUX.TCBROUTEA & ~PORTMUX_TCB1_bm) | route;
}

/**
 * @brief Route the TCB2 waveform output.
 * @param route TCB2 routing group code (`PORTMUX_TCB2_*_gc`).
 */
static inline void pmuxTcb2(PORTMUX_TCB2_t route)
{
	PORTMUX.TCBROUTEA = (PORTMUX.TCBROUTEA & ~PORTMUX_TCB2_bm) | route;
}

// TCD routing (TCDROUTEA) ----------------------------------------------------
/**
 * @brief Route the TCD0 waveform outputs.
 * @param route TCD0 routing group code (`PORTMUX_TCD0_*_gc`).
 */
static inline void pmuxTcd0(PORTMUX_TCD0_t route)
{
	PORTMUX.TCDROUTEA = (PORTMUX.TCDROUTEA & ~PORTMUX_TCD0_gm) | route;
}

// Analog comparator routing (ACROUTEA) --------------------------------------
/**
 * @brief Route the AC0 output.
 * @param route AC0 routing group code (`PORTMUX_AC0_*_gc`).
 */
static inline void pmuxAc0(PORTMUX_AC0_t route)
{
	PORTMUX.ACROUTEA = (PORTMUX.ACROUTEA & ~PORTMUX_AC0_bm) | route;
}

/**
 * @brief Route the AC1 output.
 * @param route AC1 routing group code (`PORTMUX_AC1_*_gc`).
 */
static inline void pmuxAc1(PORTMUX_AC1_t route)
{
	PORTMUX.ACROUTEA = (PORTMUX.ACROUTEA & ~PORTMUX_AC1_bm) | route;
}

/**
 * @brief Route the AC2 output.
 * @param route AC2 routing group code (`PORTMUX_AC2_*_gc`).
 */
static inline void pmuxAc2(PORTMUX_AC2_t route)
{
	PORTMUX.ACROUTEA = (PORTMUX.ACROUTEA & ~PORTMUX_AC2_bm) | route;
}

// Zero-cross detector routing (ZCDROUTEA) ------------------------------------
/**
 * @brief Route the ZCD0 output.
 * @param route ZCD0 routing group code (`PORTMUX_ZCD0_*_gc`).
 */
static inline void pmuxZcd0(PORTMUX_ZCD0_t route)
{
	PORTMUX.ZCDROUTEA = (PORTMUX.ZCDROUTEA & ~PORTMUX_ZCD0_bm) | route;
}

/** @} */ // end of pmux_driver

#endif /* PMUX_H_ */
