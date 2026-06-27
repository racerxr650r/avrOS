/**
 * @file spi.h
 * @brief SPI driver — inline accessors for the AVR-Dx serial peripheral interface.
 *
 * Provides a thin, zero-overhead set of `static inline` functions that
 * manipulate the registers of a Serial Peripheral Interface (SPI). Each
 * function operates on a caller-supplied `SPI_t *` so the same driver serves
 * any SPI instance (`&SPI0`, `&SPI1`, ...).
 *
 * The driver configures host (master) or client (slave) operation, the clock
 * prescaler and double-speed, the data order and transfer mode (clock
 * polarity/phase), the slave-select behavior, the optional buffer mode, the
 * interrupts, and the data register. A blocking `spiTransferByte()` convenience
 * performs a full-duplex byte exchange in normal (non-buffered) host mode.
 *
 * @note In normal mode `SPI_IF_bm`/`SPI_WRCOL_bm` are the active INTFLAGS bits;
 *       in buffer mode the flags are `SPI_RXCIF_bm`, `SPI_TXCIF_bm`,
 *       `SPI_DREIF_bm`, `SPI_SSIF_bm`, and `SPI_BUFOVF_bm`. The upper two bits
 *       are aliased between the two modes.
 *
 * ### Interrupt safety
 *
 * All SPI registers are 8-bit. The configuration and interrupt-enable accessors
 * are read-modify-write; serialize with an `ATOMIC_BLOCK` (see
 * `<util/atomic.h>`) if an instance is reconfigured from both main-line code and
 * an ISR. The data and flag accessors are single-register operations. There is
 * no byte-pair (`TEMP`) hazard.
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

#ifndef SPI_H_
#define SPI_H_

/** @addtogroup spi_driver
 * @{
 */

// Includes -------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>

// Data Types -----------------------------------------------------------------
/**
 * @brief SPI interrupt-enable selector (INTCTRL).
 *
 * Bit mask identifying which SPI interrupt-enable bits to set or clear. Values
 * may be OR'd together. In normal mode only `SPI_INT_NORMAL` applies; in buffer
 * mode use the individual buffer-mode interrupts.
 */
typedef enum
{
	SPI_INT_NORMAL = SPI_IE_bm,	///< Normal-mode global SPI interrupt
	SPI_INT_SS     = SPI_SSIE_bm,	///< Slave-select trigger interrupt (buffer mode)
	SPI_INT_DRE    = SPI_DREIE_bm,	///< Data register empty interrupt (buffer mode)
	SPI_INT_TXC    = SPI_TXCIE_bm,	///< Transfer complete interrupt (buffer mode)
	SPI_INT_RXC    = SPI_RXCIE_bm	///< Receive complete interrupt (buffer mode)
} spiInt_t;

// Inline Functions -----------------------------------------------------------
// Run control and configuration ----------------------------------------------
/**
 * @brief Enable the SPI module.
 *
 * Sets the ENABLE bit in CTRLA. Configure mode, role, and clock before enabling.
 *
 * @param spi Pointer to the SPI peripheral.
 */
static inline void spiEnable(SPI_t *spi)
{
	spi->CTRLA |= SPI_ENABLE_bm;
}

/**
 * @brief Disable the SPI module.
 *
 * Clears the ENABLE bit in CTRLA.
 *
 * @param spi Pointer to the SPI peripheral.
 */
static inline void spiDisable(SPI_t *spi)
{
	spi->CTRLA &= ~SPI_ENABLE_bm;
}

/**
 * @brief Select host (master) or client (slave) operation.
 *
 * Controls the MASTER bit in CTRLA.
 *
 * @param spi  Pointer to the SPI peripheral.
 * @param host true for host (master) mode, false for client (slave) mode.
 */
static inline void spiHostMode(SPI_t *spi, bool host)
{
	if(host)
		spi->CTRLA |= SPI_MASTER_bm;
	else
		spi->CTRLA &= ~SPI_MASTER_bm;
}

/**
 * @brief Select the host clock prescaler.
 *
 * Writes the PRESC field of CTRLA, dividing CLK_PER down to the SCK frequency
 * (host mode). Combine with `spiDoubleSpeed()` to halve the divisor. Other
 * CTRLA bits are preserved.
 *
 * @param spi       Pointer to the SPI peripheral.
 * @param prescaler Prescaler group code (`SPI_PRESC_*_gc`).
 */
static inline void spiSetPrescaler(SPI_t *spi, SPI_PRESC_t prescaler)
{
	spi->CTRLA = (spi->CTRLA & ~SPI_PRESC_gm) | prescaler;
}

/**
 * @brief Enable or disable double-speed clocking.
 *
 * Controls the CLK2X bit in CTRLA, halving the effective prescaler divisor in
 * host mode.
 *
 * @param spi    Pointer to the SPI peripheral.
 * @param enable true to double the clock speed, false for normal speed.
 */
static inline void spiDoubleSpeed(SPI_t *spi, bool enable)
{
	if(enable)
		spi->CTRLA |= SPI_CLK2X_bm;
	else
		spi->CTRLA &= ~SPI_CLK2X_bm;
}

/**
 * @brief Select the data bit order.
 *
 * Controls the DORD bit in CTRLA.
 *
 * @param spi      Pointer to the SPI peripheral.
 * @param lsbFirst true to transmit the LSB first, false for MSB first.
 */
static inline void spiDataOrder(SPI_t *spi, bool lsbFirst)
{
	if(lsbFirst)
		spi->CTRLA |= SPI_DORD_bm;
	else
		spi->CTRLA &= ~SPI_DORD_bm;
}

/**
 * @brief Select the transfer mode (clock polarity and phase).
 *
 * Writes the MODE field of CTRLB (SPI modes 0–3). Other CTRLB bits are
 * preserved.
 *
 * @param spi  Pointer to the SPI peripheral.
 * @param mode SPI mode group code (`SPI_MODE_*_gc`).
 */
static inline void spiSetMode(SPI_t *spi, SPI_MODE_t mode)
{
	spi->CTRLB = (spi->CTRLB & ~SPI_MODE_gm) | mode;
}

/**
 * @brief Enable or disable the slave-select (SS) line control.
 *
 * Controls the SSD bit in CTRLB. When set in host mode, the SS pin is released
 * for use as a general-purpose pin and does not force client mode.
 *
 * @param spi     Pointer to the SPI peripheral.
 * @param disable true to disable SS multi-host detection, false to keep it.
 */
static inline void spiSlaveSelectDisable(SPI_t *spi, bool disable)
{
	if(disable)
		spi->CTRLB |= SPI_SSD_bm;
	else
		spi->CTRLB &= ~SPI_SSD_bm;
}

/**
 * @brief Enable or disable buffer mode.
 *
 * Controls the BUFEN bit in CTRLB, enabling the double-buffered transmit/receive
 * mode with its own set of interrupt flags.
 *
 * @param spi    Pointer to the SPI peripheral.
 * @param enable true to enable buffer mode, false for normal mode.
 */
static inline void spiBufferEnable(SPI_t *spi, bool enable)
{
	if(enable)
		spi->CTRLB |= SPI_BUFEN_bm;
	else
		spi->CTRLB &= ~SPI_BUFEN_bm;
}

// Interrupt control ----------------------------------------------------------
/**
 * @brief Enable one or more SPI interrupts (INTCTRL).
 *
 * Sets the selected interrupt-enable bits in INTCTRL without disturbing others.
 *
 * @param spi  Pointer to the SPI peripheral.
 * @param mask Interrupt(s) to enable (`spiInt_t`, may be OR'd).
 */
static inline void spiEnableInterrupt(SPI_t *spi, spiInt_t mask)
{
	spi->INTCTRL |= mask;
}

/**
 * @brief Disable one or more SPI interrupts (INTCTRL).
 *
 * Clears the selected interrupt-enable bits in INTCTRL.
 *
 * @param spi  Pointer to the SPI peripheral.
 * @param mask Interrupt(s) to disable (`spiInt_t`, may be OR'd).
 */
static inline void spiDisableInterrupt(SPI_t *spi, spiInt_t mask)
{
	spi->INTCTRL &= ~mask;
}

/**
 * @brief Read the interrupt flags (INTFLAGS).
 *
 * Returns the raw INTFLAGS register. Interpret the bits per the active mode
 * (normal: `SPI_IF_bm`/`SPI_WRCOL_bm`; buffer: `SPI_RXCIF_bm` etc.).
 *
 * @param spi Pointer to the SPI peripheral.
 * @return The INTFLAGS register value.
 */
static inline uint8_t spiGetInterruptFlags(const SPI_t *spi)
{
	return spi->INTFLAGS;
}

/**
 * @brief Clear one or more buffer-mode interrupt flags.
 *
 * Writes a one to the selected write-one-to-clear flags in INTFLAGS (buffer
 * mode). In normal mode the IF flag is cleared by reading INTFLAGS then
 * accessing DATA.
 *
 * @param spi  Pointer to the SPI peripheral.
 * @param mask Flag bit(s) to clear.
 */
static inline void spiClearInterruptFlags(SPI_t *spi, uint8_t mask)
{
	spi->INTFLAGS = mask;
}

// Data transfer --------------------------------------------------------------
/**
 * @brief Write a byte to the data register (DATA).
 *
 * In host mode this starts a transfer; in client mode it loads the next byte to
 * shift out.
 *
 * @param spi  Pointer to the SPI peripheral.
 * @param data Byte to load.
 */
static inline void spiWriteData(SPI_t *spi, uint8_t data)
{
	spi->DATA = data;
}

/**
 * @brief Read a byte from the data register (DATA).
 *
 * @param spi Pointer to the SPI peripheral.
 * @return The received byte.
 */
static inline uint8_t spiReadData(SPI_t *spi)
{
	return spi->DATA;
}

/**
 * @brief Perform a blocking full-duplex byte exchange (normal host mode).
 *
 * Writes @p data, busy-waits for the normal-mode interrupt flag (`SPI_IF_bm`),
 * then reads and returns the received byte (which also clears the flag). For use
 * in normal (non-buffered) host mode with the SPI enabled.
 *
 * @param spi  Pointer to the SPI peripheral.
 * @param data Byte to transmit.
 * @return The byte received during the exchange.
 */
static inline uint8_t spiTransferByte(SPI_t *spi, uint8_t data)
{
	spi->DATA = data;
	while(!(spi->INTFLAGS & SPI_IF_bm))
		;
	return spi->DATA;
}

/** @} */ // end of spi_driver

#endif /* SPI_H_ */
