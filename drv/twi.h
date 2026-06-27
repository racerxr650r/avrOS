/**
 * @file twi.h
 * @brief TWI driver — inline accessors for the AVR-Dx two-wire (I2C) interface.
 *
 * Provides a thin, zero-overhead set of `static inline` functions that
 * manipulate the registers of a Two-Wire Interface (TWI). Each function
 * operates on a caller-supplied `TWI_t *` so the same driver serves any TWI
 * instance (`&TWI0`, `&TWI1`, ...).
 *
 * The TWI peripheral contains an independent **host** (master) and **client**
 * (slave) controller sharing common pin/timing configuration:
 *  - General configuration: SDA hold/setup time, fast-mode-plus, input level,
 *    dual mode, and debug behavior (functions named `twi*`).
 *  - Host controller: enable, baud, timeout, smart/quick-command modes,
 *    command strobes, the target address (which starts a transaction), the data
 *    register, and host status (`twiHost*`).
 *  - Client controller: enable, address and address mask, interrupt enables,
 *    command strobes, the data register, and client status (`twiClient*`).
 *
 * @note The host address register (MADDR) and client address register (SADDR)
 *       hold the 7-bit address in bits [7:1]; MADDR bit 0 is the read/write
 *       direction and writing MADDR initiates a transaction, while SADDR bit 0
 *       enables general-call recognition. The setters write these registers
 *       verbatim.
 *
 * ### Interrupt safety
 *
 * All TWI registers are 8-bit. The configuration and interrupt-enable accessors
 * are read-modify-write; serialize with an `ATOMIC_BLOCK` (see
 * `<util/atomic.h>`) if an instance is reconfigured from both main-line code and
 * an ISR. Command, data, and status accessors are single-register operations.
 * There is no byte-pair (`TEMP`) hazard.
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

#ifndef TWI_H_
#define TWI_H_

/** @addtogroup twi_driver
 * @{
 */

// Includes -------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>

// ===========================================================================
// General configuration
// ===========================================================================
/**
 * @brief Select the SDA hold time.
 *
 * Writes the SDAHOLD field of CTRLA. Other CTRLA bits are preserved.
 *
 * @param twi  Pointer to the TWI peripheral.
 * @param hold SDA hold-time group code (`TWI_SDAHOLD_*_gc`).
 */
static inline void twiSetSdaHold(TWI_t *twi, TWI_SDAHOLD_t hold)
{
	twi->CTRLA = (twi->CTRLA & ~TWI_SDAHOLD_gm) | hold;
}

/**
 * @brief Select the SDA setup time.
 *
 * Writes the SDASETUP bit of CTRLA. Other CTRLA bits are preserved.
 *
 * @param twi   Pointer to the TWI peripheral.
 * @param setup SDA setup-time group code (`TWI_SDASETUP_*_gc`).
 */
static inline void twiSetSdaSetup(TWI_t *twi, TWI_SDASETUP_t setup)
{
	twi->CTRLA = (twi->CTRLA & ~TWI_SDASETUP_bm) | setup;
}

/**
 * @brief Enable or disable Fast-mode Plus (1 MHz) operation.
 *
 * Controls the FMPEN bit in CTRLA.
 *
 * @param twi    Pointer to the TWI peripheral.
 * @param enable true to enable Fast-mode Plus, false for standard/fast mode.
 */
static inline void twiFastModePlus(TWI_t *twi, bool enable)
{
	if(enable)
		twi->CTRLA |= TWI_FMPEN_bm;
	else
		twi->CTRLA &= ~TWI_FMPEN_bm;
}

/**
 * @brief Select the input voltage transition level.
 *
 * Writes the INPUTLVL bit of CTRLA (I2C or SMBus 3.0 thresholds). Other CTRLA
 * bits are preserved.
 *
 * @param twi   Pointer to the TWI peripheral.
 * @param level Input-level group code (`TWI_INPUTLVL_*_gc`).
 */
static inline void twiSetInputLevel(TWI_t *twi, TWI_INPUTLVL_t level)
{
	twi->CTRLA = (twi->CTRLA & ~TWI_INPUTLVL_bm) | level;
}

/**
 * @brief Enable or disable dual-mode operation.
 *
 * Controls the ENABLE bit in DUALCTRL, which lets the client use a separate pin
 * pair from the host.
 *
 * @param twi    Pointer to the TWI peripheral.
 * @param enable true to enable dual mode, false to disable it.
 */
static inline void twiDualModeEnable(TWI_t *twi, bool enable)
{
	if(enable)
		twi->DUALCTRL |= TWI_ENABLE_bm;
	else
		twi->DUALCTRL &= ~TWI_ENABLE_bm;
}

/**
 * @brief Enable or disable TWI operation while halted in debug.
 *
 * Controls the DBGRUN bit in DBGCTRL.
 *
 * @param twi    Pointer to the TWI peripheral.
 * @param enable true to keep the TWI running while the debugger has the CPU
 *               halted, false to freeze it.
 */
static inline void twiDebugRun(TWI_t *twi, bool enable)
{
	if(enable)
		twi->DBGCTRL |= TWI_DBGRUN_bm;
	else
		twi->DBGCTRL &= ~TWI_DBGRUN_bm;
}

// ===========================================================================
// Host (master) controller
// ===========================================================================
// Run control ----------------------------------------------------------------
/**
 * @brief Enable or disable the host controller.
 *
 * Controls the ENABLE bit in MCTRLA.
 *
 * @param twi    Pointer to the TWI peripheral.
 * @param enable true to enable the host, false to disable it.
 */
static inline void twiHostEnable(TWI_t *twi, bool enable)
{
	if(enable)
		twi->MCTRLA |= TWI_ENABLE_bm;
	else
		twi->MCTRLA &= ~TWI_ENABLE_bm;
}

/**
 * @brief Enable or disable host smart mode.
 *
 * Controls the SMEN bit in MCTRLA, which auto-sends the configured ACK/NACK on
 * reading the data register.
 *
 * @param twi    Pointer to the TWI peripheral.
 * @param enable true to enable smart mode, false to disable it.
 */
static inline void twiHostSmartMode(TWI_t *twi, bool enable)
{
	if(enable)
		twi->MCTRLA |= TWI_SMEN_bm;
	else
		twi->MCTRLA &= ~TWI_SMEN_bm;
}

/**
 * @brief Select the inactive-bus timeout.
 *
 * Writes the TIMEOUT field of MCTRLA (used for SMBus bus-idle detection). Other
 * MCTRLA bits are preserved.
 *
 * @param twi     Pointer to the TWI peripheral.
 * @param timeout Timeout group code (`TWI_TIMEOUT_*_gc`).
 */
static inline void twiHostSetTimeout(TWI_t *twi, TWI_TIMEOUT_t timeout)
{
	twi->MCTRLA = (twi->MCTRLA & ~TWI_TIMEOUT_gm) | timeout;
}

/**
 * @brief Enable or disable host quick-command mode.
 *
 * Controls the QCEN bit in MCTRLA.
 *
 * @param twi    Pointer to the TWI peripheral.
 * @param enable true to enable quick command, false to disable it.
 */
static inline void twiHostQuickCommand(TWI_t *twi, bool enable)
{
	if(enable)
		twi->MCTRLA |= TWI_QCEN_bm;
	else
		twi->MCTRLA &= ~TWI_QCEN_bm;
}

/**
 * @brief Enable or disable the host read interrupt.
 *
 * Controls the RIEN bit in MCTRLA (fires when a byte has been received).
 *
 * @param twi    Pointer to the TWI peripheral.
 * @param enable true to enable the read interrupt, false to disable it.
 */
static inline void twiHostEnableReadInterrupt(TWI_t *twi, bool enable)
{
	if(enable)
		twi->MCTRLA |= TWI_RIEN_bm;
	else
		twi->MCTRLA &= ~TWI_RIEN_bm;
}

/**
 * @brief Enable or disable the host write interrupt.
 *
 * Controls the WIEN bit in MCTRLA (fires when a byte has been transmitted).
 *
 * @param twi    Pointer to the TWI peripheral.
 * @param enable true to enable the write interrupt, false to disable it.
 */
static inline void twiHostEnableWriteInterrupt(TWI_t *twi, bool enable)
{
	if(enable)
		twi->MCTRLA |= TWI_WIEN_bm;
	else
		twi->MCTRLA &= ~TWI_WIEN_bm;
}

// Commands and data ----------------------------------------------------------
/**
 * @brief Issue a host command.
 *
 * Writes the MCMD field of MCTRLB to issue a repeated-start, byte
 * receive/transmit, or stop condition. Other MCTRLB bits are preserved.
 *
 * @param twi     Pointer to the TWI peripheral.
 * @param command Host command group code (`TWI_MCMD_*_gc`).
 */
static inline void twiHostCommand(TWI_t *twi, TWI_MCMD_t command)
{
	twi->MCTRLB = (twi->MCTRLB & ~TWI_MCMD_gm) | command;
}

/**
 * @brief Select the host acknowledge action.
 *
 * Controls the ACKACT bit in MCTRLB — the ACK or NACK sent to the client on the
 * next data access.
 *
 * @param twi  Pointer to the TWI peripheral.
 * @param nack true to send NACK, false to send ACK.
 */
static inline void twiHostAckAction(TWI_t *twi, bool nack)
{
	if(nack)
		twi->MCTRLB |= TWI_ACKACT_bm;
	else
		twi->MCTRLB &= ~TWI_ACKACT_bm;
}

/**
 * @brief Flush the host state machine.
 *
 * Sets the FLUSH bit in MCTRLB, resetting the host controller's internal state.
 *
 * @param twi Pointer to the TWI peripheral.
 */
static inline void twiHostFlush(TWI_t *twi)
{
	twi->MCTRLB |= TWI_FLUSH_bm;
}

/**
 * @brief Set the host baud rate register (MBAUD).
 *
 * Writes the pre-computed MBAUD value that, with the peripheral clock, sets the
 * SCL frequency.
 *
 * @param twi  Pointer to the TWI peripheral.
 * @param baud Value to load into MBAUD.
 */
static inline void twiHostSetBaud(TWI_t *twi, uint8_t baud)
{
	twi->MBAUD = baud;
}

/**
 * @brief Write the host address register (MADDR) and start a transaction.
 *
 * Writes MADDR with the addressed-client byte: bits [7:1] are the 7-bit address
 * and bit 0 is the read/write direction (1 = read). Writing this register issues
 * a START followed by the address on the bus.
 *
 * @param twi     Pointer to the TWI peripheral.
 * @param address Address byte (address in [7:1], direction in bit 0).
 */
static inline void twiHostSetAddress(TWI_t *twi, uint8_t address)
{
	twi->MADDR = address;
}

/**
 * @brief Write a byte to the host data register (MDATA).
 *
 * @param twi  Pointer to the TWI peripheral.
 * @param data Byte to transmit.
 */
static inline void twiHostWriteData(TWI_t *twi, uint8_t data)
{
	twi->MDATA = data;
}

/**
 * @brief Read a byte from the host data register (MDATA).
 *
 * @param twi Pointer to the TWI peripheral.
 * @return The received byte.
 */
static inline uint8_t twiHostReadData(TWI_t *twi)
{
	return twi->MDATA;
}

// Status ---------------------------------------------------------------------
/**
 * @brief Read the host status register (MSTATUS).
 *
 * @param twi Pointer to the TWI peripheral.
 * @return The MSTATUS register value.
 */
static inline uint8_t twiHostGetStatus(const TWI_t *twi)
{
	return twi->MSTATUS;
}

/**
 * @brief Read the current bus state.
 *
 * Returns the BUSSTATE field of MSTATUS (unknown, idle, owner, or busy).
 *
 * @param twi Pointer to the TWI peripheral.
 * @return Bus-state group code (`TWI_BUSSTATE_t`).
 */
static inline TWI_BUSSTATE_t twiHostBusState(const TWI_t *twi)
{
	return (TWI_BUSSTATE_t)(twi->MSTATUS & TWI_BUSSTATE_gm);
}

/**
 * @brief Force the bus state (MSTATUS BUSSTATE).
 *
 * Writes the BUSSTATE field of MSTATUS, typically to force the bus to idle
 * during initialization. Writing the field also clears the WIF/RIF flags.
 *
 * @param twi   Pointer to the TWI peripheral.
 * @param state Bus-state group code (`TWI_BUSSTATE_*_gc`).
 */
static inline void twiHostSetBusState(TWI_t *twi, TWI_BUSSTATE_t state)
{
	twi->MSTATUS = state;
}

/**
 * @brief Report whether the client ACKed the last byte.
 *
 * Reads the RXACK bit of MSTATUS (0 = ACK received).
 *
 * @param twi Pointer to the TWI peripheral.
 * @return true if the last byte was acknowledged, false if NACKed.
 */
static inline bool twiHostGotAck(const TWI_t *twi)
{
	return (twi->MSTATUS & TWI_RXACK_bm) == 0;
}

/**
 * @brief Clear one or more host status flags.
 *
 * Writes a one to the selected write-one-to-clear flags in MSTATUS (e.g.
 * `TWI_WIF_bm`, `TWI_RIF_bm`, `TWI_BUSERR_bm`, `TWI_ARBLOST_bm`).
 *
 * @param twi  Pointer to the TWI peripheral.
 * @param mask Flag bit(s) to clear.
 */
static inline void twiHostClearFlags(TWI_t *twi, uint8_t mask)
{
	twi->MSTATUS = mask;
}

// ===========================================================================
// Client (slave) controller
// ===========================================================================
// Run control ----------------------------------------------------------------
/**
 * @brief Enable or disable the client controller.
 *
 * Controls the ENABLE bit in SCTRLA.
 *
 * @param twi    Pointer to the TWI peripheral.
 * @param enable true to enable the client, false to disable it.
 */
static inline void twiClientEnable(TWI_t *twi, bool enable)
{
	if(enable)
		twi->SCTRLA |= TWI_ENABLE_bm;
	else
		twi->SCTRLA &= ~TWI_ENABLE_bm;
}

/**
 * @brief Enable or disable client smart mode.
 *
 * Controls the SMEN bit in SCTRLA.
 *
 * @param twi    Pointer to the TWI peripheral.
 * @param enable true to enable smart mode, false to disable it.
 */
static inline void twiClientSmartMode(TWI_t *twi, bool enable)
{
	if(enable)
		twi->SCTRLA |= TWI_SMEN_bm;
	else
		twi->SCTRLA &= ~TWI_SMEN_bm;
}

/**
 * @brief Enable or disable the client data interrupt.
 *
 * Controls the DIEN bit in SCTRLA (fires on a data byte transfer).
 *
 * @param twi    Pointer to the TWI peripheral.
 * @param enable true to enable the data interrupt, false to disable it.
 */
static inline void twiClientEnableDataInterrupt(TWI_t *twi, bool enable)
{
	if(enable)
		twi->SCTRLA |= TWI_DIEN_bm;
	else
		twi->SCTRLA &= ~TWI_DIEN_bm;
}

/**
 * @brief Enable or disable the client address/stop interrupt.
 *
 * Controls the APIEN bit in SCTRLA (fires on address match or stop condition).
 *
 * @param twi    Pointer to the TWI peripheral.
 * @param enable true to enable the address/stop interrupt, false to disable it.
 */
static inline void twiClientEnableAddressInterrupt(TWI_t *twi, bool enable)
{
	if(enable)
		twi->SCTRLA |= TWI_APIEN_bm;
	else
		twi->SCTRLA &= ~TWI_APIEN_bm;
}

/**
 * @brief Enable or disable the client stop interrupt.
 *
 * Controls the PIEN bit in SCTRLA (enables the stop condition to set APIF).
 *
 * @param twi    Pointer to the TWI peripheral.
 * @param enable true to enable the stop interrupt, false to disable it.
 */
static inline void twiClientEnableStopInterrupt(TWI_t *twi, bool enable)
{
	if(enable)
		twi->SCTRLA |= TWI_PIEN_bm;
	else
		twi->SCTRLA &= ~TWI_PIEN_bm;
}

// Commands and data ----------------------------------------------------------
/**
 * @brief Issue a client command.
 *
 * Writes the SCMD field of SCTRLB to respond to or complete a transaction.
 * Other SCTRLB bits are preserved.
 *
 * @param twi     Pointer to the TWI peripheral.
 * @param command Client command group code (`TWI_SCMD_*_gc`).
 */
static inline void twiClientCommand(TWI_t *twi, TWI_SCMD_t command)
{
	twi->SCTRLB = (twi->SCTRLB & ~TWI_SCMD_gm) | command;
}

/**
 * @brief Select the client acknowledge action.
 *
 * Controls the ACKACT bit in SCTRLB — the ACK or NACK sent on the next data
 * access.
 *
 * @param twi  Pointer to the TWI peripheral.
 * @param nack true to send NACK, false to send ACK.
 */
static inline void twiClientAckAction(TWI_t *twi, bool nack)
{
	if(nack)
		twi->SCTRLB |= TWI_ACKACT_bm;
	else
		twi->SCTRLB &= ~TWI_ACKACT_bm;
}

/**
 * @brief Set the client address (SADDR).
 *
 * Writes SADDR: bits [7:1] are the 7-bit address this client responds to and
 * bit 0 enables general-call recognition.
 *
 * @param twi     Pointer to the TWI peripheral.
 * @param address Address byte (address in [7:1], general-call enable in bit 0).
 */
static inline void twiClientSetAddress(TWI_t *twi, uint8_t address)
{
	twi->SADDR = address;
}

/**
 * @brief Configure the client address mask (SADDRMASK).
 *
 * Writes SADDRMASK. When @p secondAddress is false, the mask bits [7:1] mark
 * address bits to ignore (acting as a "don't care" mask); when true, the field
 * is a second address the client also responds to. The ADDREN bit selects
 * between the two interpretations.
 *
 * @param twi           Pointer to the TWI peripheral.
 * @param mask          Mask or second-address value in bits [7:1].
 * @param secondAddress true to treat @p mask as a second address, false to use
 *                      it as an address bit mask.
 */
static inline void twiClientSetAddressMask(TWI_t *twi, uint8_t mask, bool secondAddress)
{
	twi->SADDRMASK = (mask & TWI_ADDRMASK_gm) | (secondAddress ? TWI_ADDREN_bm : 0);
}

/**
 * @brief Write a byte to the client data register (SDATA).
 *
 * @param twi  Pointer to the TWI peripheral.
 * @param data Byte to transmit to the host.
 */
static inline void twiClientWriteData(TWI_t *twi, uint8_t data)
{
	twi->SDATA = data;
}

/**
 * @brief Read a byte from the client data register (SDATA).
 *
 * @param twi Pointer to the TWI peripheral.
 * @return The received byte.
 */
static inline uint8_t twiClientReadData(TWI_t *twi)
{
	return twi->SDATA;
}

// Status ---------------------------------------------------------------------
/**
 * @brief Read the client status register (SSTATUS).
 *
 * @param twi Pointer to the TWI peripheral.
 * @return The SSTATUS register value.
 */
static inline uint8_t twiClientGetStatus(const TWI_t *twi)
{
	return twi->SSTATUS;
}

/**
 * @brief Report the addressed transfer direction.
 *
 * Reads the DIR bit of SSTATUS, set when the host is reading from this client.
 *
 * @param twi Pointer to the TWI peripheral.
 * @return true if the host is reading (client transmits), false if writing.
 */
static inline bool twiClientIsRead(const TWI_t *twi)
{
	return (twi->SSTATUS & TWI_DIR_bm) != 0;
}

/**
 * @brief Report whether the host ACKed the last byte the client sent.
 *
 * Reads the RXACK bit of SSTATUS (0 = ACK received).
 *
 * @param twi Pointer to the TWI peripheral.
 * @return true if the last byte was acknowledged, false if NACKed.
 */
static inline bool twiClientGotAck(const TWI_t *twi)
{
	return (twi->SSTATUS & TWI_RXACK_bm) == 0;
}

/**
 * @brief Clear one or more client status flags.
 *
 * Writes a one to the selected write-one-to-clear flags in SSTATUS (e.g.
 * `TWI_DIF_bm`, `TWI_APIF_bm`, `TWI_BUSERR_bm`, `TWI_COLL_bm`).
 *
 * @param twi  Pointer to the TWI peripheral.
 * @param mask Flag bit(s) to clear.
 */
static inline void twiClientClearFlags(TWI_t *twi, uint8_t mask)
{
	twi->SSTATUS = mask;
}

/** @} */ // end of twi_driver

#endif /* TWI_H_ */
