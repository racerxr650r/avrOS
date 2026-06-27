/**
 * @file nvm.h
 * @brief NVM driver — inline accessors for the AVR-Dx non-volatile memory controller.
 *
 * Provides a thin, zero-overhead set of `static inline` functions that
 * manipulate the registers of the Non-Volatile Memory Controller (NVMCTRL). The
 * AVR-Dx has a single NVM controller, so these functions operate directly on
 * the global `NVMCTRL` register set rather than on a caller-supplied pointer.
 *
 * The driver issues flash/EEPROM commands, polls the busy/ready and error
 * status, controls the EEPROM-ready interrupt, and configures the flash mapping
 * of program memory into the data address space.
 *
 * On the AVR-Dx, flash and EEPROM are programmed by first writing data to the
 * memory-mapped program/EEPROM address space (loading the page buffer), then
 * issuing the appropriate command through `nvmCommand()`. This driver provides
 * the controller primitives; the page-buffer writes and address handling are the
 * caller's responsibility.
 *
 * @note The command register (CTRLA) is protected by the Configuration Change
 *       Protection (CCP) mechanism using the self-programming (SPM) key.
 *       `nvmCommand()` performs this via avr-libc's `ccp_write_spm()`. Always
 *       wait for the controller to be idle (`nvmWaitReady()`) before issuing a
 *       new command.
 *
 * ### Interrupt safety
 *
 * Self-programming is normally driven from a single context. The flash-map
 * setter (`nvmSetFlashMap()`) is an 8-bit read-modify-write; serialize with an
 * `ATOMIC_BLOCK` (see `<util/atomic.h>`) if used from both main-line code and an
 * ISR. The status, command, and interrupt-flag accessors carry no byte-pair
 * (`TEMP`) hazard.
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

#ifndef NVM_H_
#define NVM_H_

/** @addtogroup nvm_driver
 * @{
 */

// Includes -------------------------------------------------------------------
#include <stdint.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/cpufunc.h>

// Data Types -----------------------------------------------------------------
/**
 * @brief NVM busy/ready status flag selector.
 *
 * Bit mask identifying the busy flags in the STATUS register. Values may be
 * OR'd together when testing the result of `nvmGetStatus()`.
 */
typedef enum
{
	NVM_BUSY_FLASH  = NVMCTRL_FBUSY_bm,	///< A flash operation is in progress
	NVM_BUSY_EEPROM = NVMCTRL_EEBUSY_bm,	///< An EEPROM operation is in progress
	NVM_BUSY_ANY    = NVMCTRL_FBUSY_bm | NVMCTRL_EEBUSY_bm	///< Flash or EEPROM busy
} nvmBusy_t;

// Inline Functions -----------------------------------------------------------
// Command execution ----------------------------------------------------------
/**
 * @brief Issue an NVM command.
 *
 * Writes @p command to the CTRLA command register through the CCP SPM-protected
 * write sequence, starting a flash or EEPROM operation. Ensure the controller is
 * idle (`nvmWaitReady()`) and any required page-buffer data has been written
 * before calling.
 *
 * @param command NVM command group code (`NVMCTRL_CMD_*_gc`).
 */
static inline void nvmCommand(NVMCTRL_CMD_t command)
{
	ccp_write_spm((void *)&NVMCTRL.CTRLA, command);
}

/**
 * @brief Clear any pending NVM command.
 *
 * Writes the NONE command to CTRLA (CCP SPM-protected), returning the controller
 * to an idle command state.
 */
static inline void nvmClearCommand(void)
{
	ccp_write_spm((void *)&NVMCTRL.CTRLA, NVMCTRL_CMD_NONE_gc);
}

// Status ---------------------------------------------------------------------
/**
 * @brief Read the NVM busy status flags.
 *
 * Returns the STATUS register masked to the busy flags. Test against
 * `NVM_BUSY_FLASH` / `NVM_BUSY_EEPROM`.
 *
 * @return Bit mask of active busy flags (`nvmBusy_t`).
 */
static inline nvmBusy_t nvmGetStatus(void)
{
	return (nvmBusy_t)(NVMCTRL.STATUS & NVM_BUSY_ANY);
}

/**
 * @brief Report whether a flash operation is in progress.
 *
 * @return true if flash is busy, false otherwise.
 */
static inline bool nvmFlashBusy(void)
{
	return (NVMCTRL.STATUS & NVMCTRL_FBUSY_bm) != 0;
}

/**
 * @brief Report whether an EEPROM operation is in progress.
 *
 * @return true if EEPROM is busy, false otherwise.
 */
static inline bool nvmEepromBusy(void)
{
	return (NVMCTRL.STATUS & NVMCTRL_EEBUSY_bm) != 0;
}

/**
 * @brief Block until the controller is idle.
 *
 * Spins while either the flash or EEPROM busy flag is set in STATUS. Call before
 * writing the page buffer or issuing a new command.
 */
static inline void nvmWaitReady(void)
{
	while(NVMCTRL.STATUS & NVM_BUSY_ANY)
		;
}

/**
 * @brief Read the last write-error code.
 *
 * Returns the ERROR field of STATUS, which records why the most recent write was
 * rejected (illegal command, illegal section, double-select, or programming
 * collision). `NVMCTRL_ERROR_NOERROR_gc` indicates no error.
 *
 * @return The error group code (`NVMCTRL_ERROR_t`).
 */
static inline NVMCTRL_ERROR_t nvmGetError(void)
{
	return (NVMCTRL_ERROR_t)(NVMCTRL.STATUS & NVMCTRL_ERROR_gm);
}

// EEPROM-ready interrupt -----------------------------------------------------
/**
 * @brief Enable or disable the EEPROM-ready interrupt.
 *
 * Controls the EEREADY bit in INTCTRL. The interrupt fires (level, not edge)
 * whenever the EEPROM is ready for new data.
 *
 * @param enable true to enable the interrupt, false to disable it.
 */
static inline void nvmEnableEepromReadyInterrupt(bool enable)
{
	if(enable)
		NVMCTRL.INTCTRL |= NVMCTRL_EEREADY_bm;
	else
		NVMCTRL.INTCTRL &= ~NVMCTRL_EEREADY_bm;
}

/**
 * @brief Report whether the EEPROM-ready interrupt flag is set.
 *
 * Reads the EEREADY bit of INTFLAGS. This flag is cleared by hardware when the
 * EEPROM becomes busy again, not by writing to it.
 *
 * @return true if the EEPROM-ready flag is set, false otherwise.
 */
static inline bool nvmEepromReady(void)
{
	return (NVMCTRL.INTFLAGS & NVMCTRL_EEREADY_bm) != 0;
}

// Flash mapping --------------------------------------------------------------
/**
 * @brief Select which flash section is mapped into the data address space.
 *
 * Writes the FLMAP field of CTRLB, choosing which 32 KB flash section appears in
 * the data space for memory-mapped reads. Other CTRLB bits are preserved. Has no
 * effect once the mapping is locked (`nvmLockFlashMap()`).
 *
 * @param section Flash-section group code (`NVMCTRL_FLMAP_*_gc`).
 */
static inline void nvmSetFlashMap(NVMCTRL_FLMAP_t section)
{
	NVMCTRL.CTRLB = (NVMCTRL.CTRLB & ~NVMCTRL_FLMAP_gm) | section;
}

/**
 * @brief Lock the flash-mapping selection until the next reset.
 *
 * Sets the FLMAPLOCK bit in CTRLB. Once set, the FLMAP field cannot be changed
 * until a reset occurs.
 */
static inline void nvmLockFlashMap(void)
{
	NVMCTRL.CTRLB |= NVMCTRL_FLMAPLOCK_bm;
}

/** @} */ // end of nvm_driver

#endif /* NVM_H_ */
