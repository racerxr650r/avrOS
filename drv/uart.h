/**
 * @file uart.h
 * @brief UART driver — interrupt-driven USART with queue buffering and FILE stream integration.
 *
 * Types, constants, macros, and function prototypes for a standard
 * asynchronous receiver/transmitter using the AVR-Dx USART
 *
 * Created: 2/28/2021 4:14:29 PM
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


#ifndef UART_H_
#define UART_H_

/** @addtogroup uart_driver
 * @{
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <avr/io.h>
#include "queue.h"

// Data Types -----------------------------------------------------------------
typedef struct
{
	uint32_t	txBytes;
	uint32_t	rxBytes;
	uint32_t	txQueueOverflow;
	uint32_t	rxBufferOverflow;
	uint32_t	rxQueueOverflow;
	uint32_t	frameError;
	uint32_t	parityError;
}UartStats_t;

typedef struct
{
#ifdef UART_STATS
	char			 *name;			///< Name of the UART instance
#endif
	const FILE*		 file;			///< Stream I/O file pointer for the buffered device
	USART_t			 *usartRegs;	///< USART device
	uint16_t		 baud;			///< Baud rate / 100
	USART_PMODE_t	 parity;		///< Parity
	USART_CHSIZE_t	 dataBits;		///< Data bits
	USART_SBMODE_t	 stopBits;		///< Stop bits
	volatile queue_t *txQueue;		///< Transmit Queue
	volatile queue_t *rxQueue;		///< Transmit Queue
#ifdef UART_STATS
	UartStats_t		*stats;			///< Device statistics
#endif
}UART_t;


// Uart Macros ----------------------------------------------------------------
/**
 * @brief Get the FILE symbol name associated with a UART instance name.
 *
 * @param uartName UART instance symbol.
 * @return Concatenated symbol `uartName_file`.
 */
#define UART_FILE_PTR(uartName)	CONCAT(uartName,_file)

#ifdef UART_STATS
// Verbose version of the UART_ADD macros. These versions of the macros save
// the name of the UART and create statistics that can be used by the
// application, cli, or modbus server
/**
 * @brief Add a buffered read/write UART instance.
 *
 * Creates TX and RX queues, FILE stream bindings, UART descriptor data, and a
 * startup initializer entry for `uartInit`.
 *
 * @param usartName UART instance symbol name.
 * @param usartReg USART peripheral register block (for example USART0).
 * @param uartBaud UART baud rate in bits per second.
 * @param uartParity UART parity configuration.
 * @param uartDataBits UART character size configuration.
 * @param uartStopBits UART stop-bit configuration.
 * @param txQueueSize Transmit queue capacity in bytes.
 * @param rxQueueSize Receive queue capacity in bytes.
 */
#define ADD_UART_RW(usartName, usartReg, uartBaud, uartParity, uartDataBits, uartStopBits, txQueueSize, rxQueueSize) \
				ADD_QUEUE(usartName ## _TxQue,sizeof(uint8_t),txQueueSize); \
				ADD_QUEUE(usartName ## _RxQue,sizeof(uint8_t),rxQueueSize); \
				static UartStats_t CONCAT(usartName,_stats); \
				const static UART_t SECTION(UART_TABLE) usartName; \
				const static fioBuffers_t CONCAT(usartName,_buffers) = {.input = &CONCAT(usartName,_RxQue), .output = &CONCAT(usartName,_TxQue)}; \
				static FILE CONCAT(usartName,_file) = {.buf = (char *) &CONCAT(usartName,_buffers), .put = uartPutChar, .get = uartGetChar, .flags = _FDEV_SETUP_RW, .udata = (void *)&usartName }; \
				const static UART_t SECTION(UART_TABLE) usartName = { .name = #usartName, .file = &CONCAT(usartName,_file), .usartRegs = &usartReg, .baud = (uartBaud/100), .parity = uartParity, .dataBits = uartDataBits, .stopBits = uartStopBits, .txQueue = &CONCAT(usartName,_TxQue), .rxQueue = &CONCAT(usartName,_RxQue), .stats = &CONCAT(usartName,_stats)}; \
				ADD_INITIALIZER(usartName,uartInit,(void *)&usartName);
/**
 * @brief Add a buffered write-only UART instance.
 *
 * Creates a TX queue, FILE stream binding, UART descriptor data, and a startup
 * initializer entry for `uartInit`.
 *
 * @param usartName UART instance symbol name.
 * @param usartReg USART peripheral register block (for example USART0).
 * @param uartBaud UART baud rate in bits per second.
 * @param uartParity UART parity configuration.
 * @param uartDataBits UART character size configuration.
 * @param uartStopBits UART stop-bit configuration.
 * @param txQueueSize Transmit queue capacity in bytes.
 */
#define ADD_UART_WRITE(usartName, usartReg, uartBaud, uartParity, uartDataBits, uartStopBits, txQueueSize) \
				ADD_QUEUE(usartName ## _TxQue,sizeof(uint8_t),txQueueSize); \
				static UartStats_t CONCAT(usartName,_stats); \
				const static UART_t SECTION(UART_TABLE) usartName; \
				const static fioBuffers_t CONCAT(usartName,_buffers) = {.input = NULL, .output = &CONCAT(usartName,_TxQue)}; \
				static FILE CONCAT(usartName,_file) = {.buf = (char *) &CONCAT(usartName,_buffers), .put = uartPutChar, .get = uartGetChar, .flags = _FDEV_SETUP_WRITE, .udata = (void *)&usartName }; \
				const static UART_t SECTION(UART_TABLE) usartName = { .name = #usartName, .file = &CONCAT(usartName,_file), .usartRegs = &usartReg, .baud = (uartBaud/100), .parity = uartParity, .dataBits = uartDataBits, .stopBits = uartStopBits, .txQueue = &CONCAT(usartName,_TxQue), .rxQueue = NULL, .stats = &CONCAT(usartName,_stats)}; \
				ADD_INITIALIZER(usartName,uartInit,(void *)&usartName);
/**
 * @brief Add a buffered read-only UART instance.
 *
 * Creates an RX queue, FILE stream binding, UART descriptor data, and a startup
 * initializer entry for `uartInit`.
 *
 * @param usartName UART instance symbol name.
 * @param usartReg USART peripheral register block (for example USART0).
 * @param uartBaud UART baud rate in bits per second.
 * @param uartParity UART parity configuration.
 * @param uartDataBits UART character size configuration.
 * @param uartStopBits UART stop-bit configuration.
 * @param rxQueueSize Receive queue capacity in bytes.
 */
#define ADD_UART_READ(usartName, usartReg, uartBaud, uartParity, uartDataBits, uartStopBits, rxQueueSize) \
				ADD_QUEUE(usartName ## _RxQue,sizeof(uint8_t),rxQueueSize); \
				static UartStats_t CONCAT(usartName,_stats); \
				const static UART_t SECTION(UART_TABLE) usartName; \
				static FILE CONCAT(usartName,_file) = { .put = uartPutChar, .get = uartGetChar, .flags = _FDEV_SETUP_READ, .udata = (void *)&usartName }; \
				const static UART_t SECTION(UART_TABLE) usartName = { .name = #usartName, .file = &CONCAT(usartName,_file), .usartRegs = &usartReg, .baud = (uartBaud/100), .parity = uartParity, .dataBits = uartDataBits, .stopBits = uartStopBits, .txQueue = NULL, .rxQueue = &CONCAT(usartName,_RxQue), .stats = &CONCAT(usartName,_stats)}; \
				ADD_INITIALIZER(usartName,uartInit,(void *)&usartName);
/**
 * @brief Add an unbuffered raw UART instance.
 *
 * Creates UART descriptor data with no queues and registers a startup
 * initializer entry for `uartInit`.
 *
 * @param usartName UART instance symbol name.
 * @param usartReg USART peripheral register block (for example USART0).
 * @param uartBaud UART baud rate in bits per second.
 * @param uartParity UART parity configuration.
 * @param uartDataBits UART character size configuration.
 * @param uartStopBits UART stop-bit configuration.
 */
#define ADD_UART_RAW(usartName, usartReg, uartBaud, uartParity, uartDataBits, uartStopBits) \
				static UartStats_t CONCAT(usartName,_stats); \
				const static UART_t SECTION(UART_TABLE) usartName = { .name = #usartName, .file = &CONCAT(usartName,_file), .usartRegs = &usartReg, .baud = (uartBaud/100), .parity = uartParity, .dataBits = uartDataBits, .stopBits = uartStopBits, .txQueue = NULL, .rxQueue = NULL, .stats = &CONCAT(usartName,_stats)}; \
				ADD_INITIALIZER(usartName,uartInit,(void *)&usartName);
#else
#define ADD_UART_RW(usartName, usartReg, uartBaud, uartParity, uartDataBits, uartStopBits, txQueueSize, rxQueueSize) \
				ADD_QUEUE(usartName ## _TxQue,sizeof(uint8_t),txQueueSize); \
				ADD_QUEUE(usartName ## _RxQue,sizeof(uint8_t),rxQueueSize); \
				const static UART_t SECTION(UART_TABLE) usartName; \
				const static fioBuffers_t CONCAT(usartName,_buffers) = {.input = &CONCAT(usartName,_RxQue), .output = &CONCAT(usartName,_TxQue)}; \
				static FILE CONCAT(usartName,_file) = {.buf = (char *) &CONCAT(usartName,_buffers), .put = uartPutChar, .get = uartGetChar, .flags = _FDEV_SETUP_RW, .udata = (void *)&usartName }; \
				const static UART_t SECTION(UART_TABLE) usartName = { file = &CONCAT(usartName,_file), .usartRegs = &usartReg, .baud = (uartBaud/100), .parity = uartParity, .dataBits = uartDataBits, .stopBits = uartStopBits, .txQueue = &CONCAT(usartName,_TxQue), .rxQueue = &CONCAT(usartName,_RxQue)}; \
				ADD_INITIALIZER(usartName,uartInit,(void *)&usartName);
#define ADD_UART_WRITE(usartName, usartReg, uartBaud, uartParity, uartDataBits, uartStopBits, txQueueSize) \
				ADD_QUEUE(usartName ## _TxQue,sizeof(uint8_t),txQueueSize); \
				const static UART_t SECTION(UART_TABLE) usartName; \
				const static fioBuffers_t CONCAT(usartName,_buffers) = {.input = NULL, .output = &CONCAT(usartName,_TxQue)}; \
				static FILE CONCAT(usartName,_file) = {.buf = (char *) &CONCAT(usartName,_buffers), .put = uartPutChar, .get = uartGetChar, .flags = _FDEV_SETUP_WRITE, .udata = (void *)&usartName }; \
				const static UART_t SECTION(UART_TABLE) usartName = { .file = &CONCAT(usartName,_file), .usartRegs = &usartReg, .baud = (uartBaud/100), .parity = uartParity, .dataBits = uartDataBits, .stopBits = uartStopBits, .txQueue = &CONCAT(usartName,_TxQue), .rxQueue = NULL}; \
				ADD_INITIALIZER(usartName,uartInit,(void *)&usartName);
#define ADD_UART_READ(usartName, usartReg, uartBaud, uartParity, uartDataBits, uartStopBits, rxQueueSize) \
				ADD_QUEUE(usartName ## _RxQue,sizeof(uint8_t),rxQueueSize); \
				const static UART_t SECTION(UART_TABLE) usartName; \
				static FILE CONCAT(usartName,_file) = { .put = uartPutChar, .get = uartGetChar, .flags = _FDEV_SETUP_READ, .udata = (void *)&usartName }; \
				const static UART_t SECTION(UART_TABLE) usartName = { .file = &CONCAT(usartName,_file), .usartRegs = &usartReg, .baud = (uartBaud/100), .parity = uartParity, .dataBits = uartDataBits, .stopBits = uartStopBits, .txQueue = NULL, .rxQueue = &CONCAT(usartName,_RxQue)}; \
				ADD_INITIALIZER(usartName,uartInit,(void *)&usartName);
#define ADD_UART_RAW(usartName, usartReg, uartBaud, uartParity, uartDataBits, uartStopBits) \
				const static UART_t SECTION(UART_TABLE) usartName = { .file = &CONCAT(usartName,_file), .usartRegs = &usartReg, .baud = (uartBaud/100), .parity = uartParity, .dataBits = uartDataBits, .stopBits = uartStopBits, .txQueue = NULL, .rxQueue = NULL}; \
				ADD_INITIALIZER(usartName,uartInit,(void *)&usartName);
#endif

// Inline Register Accessors --------------------------------------------------
// Low-overhead `static inline` functions that manipulate the USART peripheral
// registers directly, operating on a caller-supplied `USART_t *`. The buffered,
// queue-based driver API below is built on top of these. Use these when adding
// new code that touches the USART registers rather than accessing them directly.
//
// Interrupt safety: the configuration accessors are 8-bit read-modify-write and
// are normally run once from `uartInit()` inside an ATOMIC_BLOCK; the data and
// status accessors are single-register reads/writes. There is no byte-pair
// (TEMP) hazard — the USART has no shared TEMP register.

/**
 * @brief USART interrupt-enable selector (CTRLA).
 *
 * Bit mask identifying which USART interrupt-enable bits to set or clear. Values
 * may be OR'd together. The bit positions match the CTRLA register.
 */
typedef enum
{
	USART_INT_RXC = USART_RXCIE_bm,	///< Receive complete interrupt
	USART_INT_TXC = USART_TXCIE_bm,	///< Transmit complete interrupt
	USART_INT_DRE = USART_DREIE_bm,	///< Data register empty interrupt
	USART_INT_RXS = USART_RXSIE_bm,	///< Receive start interrupt
	USART_INT_ABE = USART_ABEIE_bm	///< Auto-baud error interrupt
} usartInt_t;

// Configuration --------------------------------------------------------------
/**
 * @brief Set the baud rate register (BAUD).
 *
 * Writes the 16-bit BAUD register with a pre-computed value. The baud register
 * value is derived from the peripheral clock, oversampling, and target baud.
 *
 * @param usart Pointer to the USART peripheral.
 * @param baud  Value to load into the BAUD register.
 */
static inline void usartSetBaud(USART_t *usart, uint16_t baud)
{
	usart->BAUD = baud;
}

/**
 * @brief Set the asynchronous frame format (CTRLC).
 *
 * Writes CTRLC with the communication mode, parity, character size, and stop
 * bits in a single register write.
 *
 * @param usart    Pointer to the USART peripheral.
 * @param mode     Communication mode group code (`USART_CMODE_*_gc`).
 * @param parity   Parity group code (`USART_PMODE_*_gc`).
 * @param dataBits Character size group code (`USART_CHSIZE_*_gc`).
 * @param stopBits Stop-bit group code (`USART_SBMODE_*_gc`).
 */
static inline void usartSetFrameFormat(USART_t *usart, USART_CMODE_t mode, USART_PMODE_t parity, USART_CHSIZE_t dataBits, USART_SBMODE_t stopBits)
{
	usart->CTRLC = mode | parity | dataBits | stopBits;
}

/**
 * @brief Select the communication mode (CTRLC CMODE).
 *
 * Writes the CMODE field of CTRLC (asynchronous, synchronous, IRCOM, or
 * SPI-host). Other CTRLC fields are preserved. Use this to change one field
 * without rewriting the whole frame format.
 *
 * @param usart Pointer to the USART peripheral.
 * @param mode  Communication mode group code (`USART_CMODE_*_gc`).
 */
static inline void usartSetCommMode(USART_t *usart, USART_CMODE_t mode)
{
	usart->CTRLC = (usart->CTRLC & ~USART_CMODE_gm) | mode;
}

/**
 * @brief Select the parity mode (CTRLC PMODE).
 *
 * Writes the PMODE field of CTRLC (disabled, even, or odd). Other CTRLC fields
 * are preserved.
 *
 * @param usart  Pointer to the USART peripheral.
 * @param parity Parity group code (`USART_PMODE_*_gc`).
 */
static inline void usartSetParity(USART_t *usart, USART_PMODE_t parity)
{
	usart->CTRLC = (usart->CTRLC & ~USART_PMODE_gm) | parity;
}

/**
 * @brief Select the character size (CTRLC CHSIZE).
 *
 * Writes the CHSIZE field of CTRLC (5–9 data bits). Other CTRLC fields are
 * preserved.
 *
 * @param usart    Pointer to the USART peripheral.
 * @param dataBits Character size group code (`USART_CHSIZE_*_gc`).
 */
static inline void usartSetDataBits(USART_t *usart, USART_CHSIZE_t dataBits)
{
	usart->CTRLC = (usart->CTRLC & ~USART_CHSIZE_gm) | dataBits;
}

/**
 * @brief Select the number of stop bits (CTRLC SBMODE).
 *
 * Writes the SBMODE field of CTRLC (1 or 2 stop bits). Other CTRLC fields are
 * preserved.
 *
 * @param usart    Pointer to the USART peripheral.
 * @param stopBits Stop-bit group code (`USART_SBMODE_*_gc`).
 */
static inline void usartSetStopBits(USART_t *usart, USART_SBMODE_t stopBits)
{
	usart->CTRLC = (usart->CTRLC & ~USART_SBMODE_bm) | stopBits;
}

/**
 * @brief Enable or disable the receiver (CTRLB RXEN).
 *
 * @param usart  Pointer to the USART peripheral.
 * @param enable true to enable the receiver, false to disable it.
 */
static inline void usartEnableReceiver(USART_t *usart, bool enable)
{
	if(enable)
		usart->CTRLB |= USART_RXEN_bm;
	else
		usart->CTRLB &= ~USART_RXEN_bm;
}

/**
 * @brief Enable or disable the transmitter (CTRLB TXEN).
 *
 * @param usart  Pointer to the USART peripheral.
 * @param enable true to enable the transmitter, false to disable it.
 */
static inline void usartEnableTransmitter(USART_t *usart, bool enable)
{
	if(enable)
		usart->CTRLB |= USART_TXEN_bm;
	else
		usart->CTRLB &= ~USART_TXEN_bm;
}

/**
 * @brief Select the receiver oversampling mode (CTRLB RXMODE).
 *
 * Writes the RXMODE field of CTRLB (normal 16x, double-speed 8x, ...). Other
 * CTRLB bits — including the enable bits — are preserved.
 *
 * @param usart Pointer to the USART peripheral.
 * @param mode  Receiver mode group code (`USART_RXMODE_*_gc`).
 */
static inline void usartSetRxMode(USART_t *usart, USART_RXMODE_t mode)
{
	usart->CTRLB = (usart->CTRLB & ~USART_RXMODE_gm) | mode;
}

// Interrupt control ----------------------------------------------------------
/**
 * @brief Enable one or more USART interrupts (CTRLA).
 *
 * Sets the selected interrupt-enable bits in CTRLA without disturbing other
 * enabled interrupts.
 *
 * @param usart Pointer to the USART peripheral.
 * @param mask  Interrupt(s) to enable (`usartInt_t`, may be OR'd).
 */
static inline void usartEnableInterrupt(USART_t *usart, usartInt_t mask)
{
	usart->CTRLA |= mask;
}

/**
 * @brief Disable one or more USART interrupts (CTRLA).
 *
 * Clears the selected interrupt-enable bits in CTRLA.
 *
 * @param usart Pointer to the USART peripheral.
 * @param mask  Interrupt(s) to disable (`usartInt_t`, may be OR'd).
 */
static inline void usartDisableInterrupt(USART_t *usart, usartInt_t mask)
{
	usart->CTRLA &= ~mask;
}

// Data transfer --------------------------------------------------------------
/**
 * @brief Write a byte to the transmit data register (TXDATAL).
 *
 * Loads a byte for transmission. Check `usartDataRegisterEmpty()` (or use the
 * DRE interrupt) before writing to avoid overwriting unsent data.
 *
 * @param usart Pointer to the USART peripheral.
 * @param data  Byte to transmit.
 */
static inline void usartWriteData(USART_t *usart, uint8_t data)
{
	usart->TXDATAL = data;
}

/**
 * @brief Read a byte from the receive data register (RXDATAL).
 *
 * Reading RXDATAL pops the received byte from the receive FIFO. Read
 * `usartReadRxStatus()` first if the frame's error flags are needed.
 *
 * @param usart Pointer to the USART peripheral.
 * @return The received byte.
 */
static inline uint8_t usartReadData(USART_t *usart)
{
	return usart->RXDATAL;
}

/**
 * @brief Read the receive status / high byte (RXDATAH).
 *
 * Returns RXDATAH, which holds the receive-complete flag (`USART_RXCIF_bm`), the
 * per-frame error flags (`USART_BUFOVF_bm`, `USART_FERR_bm`, `USART_PERR_bm`),
 * and the ninth data bit. Read this before `usartReadData()` so the flags
 * correspond to the byte that read returns.
 *
 * @param usart Pointer to the USART peripheral.
 * @return The RXDATAH register value.
 */
static inline uint8_t usartReadRxStatus(USART_t *usart)
{
	return usart->RXDATAH;
}

// Status ---------------------------------------------------------------------
/**
 * @brief Read the status register (STATUS).
 *
 * @param usart Pointer to the USART peripheral.
 * @return The STATUS register value (`USART_*IF_bm` flags).
 */
static inline uint8_t usartGetStatus(const USART_t *usart)
{
	return usart->STATUS;
}

/**
 * @brief Report whether a received byte is available.
 *
 * Reads the RXCIF flag in RXDATAH. Does not pop the FIFO (only `usartReadData()`
 * does).
 *
 * @param usart Pointer to the USART peripheral.
 * @return true if a byte has been received, false otherwise.
 */
static inline bool usartRxComplete(USART_t *usart)
{
	return (usart->RXDATAH & USART_RXCIF_bm) != 0;
}

/**
 * @brief Report whether the transmit data register is empty.
 *
 * Reads the DREIF flag in STATUS, indicating TXDATAL can accept a new byte.
 *
 * @param usart Pointer to the USART peripheral.
 * @return true if the transmit data register is ready, false otherwise.
 */
static inline bool usartDataRegisterEmpty(const USART_t *usart)
{
	return (usart->STATUS & USART_DREIF_bm) != 0;
}

/**
 * @brief Report whether a transmission has completed.
 *
 * Reads the TXCIF flag in STATUS, set when the entire frame (including stop
 * bits) has been shifted out and no new data is in the buffer.
 *
 * @param usart Pointer to the USART peripheral.
 * @return true if transmission is complete, false otherwise.
 */
static inline bool usartTransmitComplete(const USART_t *usart)
{
	return (usart->STATUS & USART_TXCIF_bm) != 0;
}

// External Functions ---------------------------------------------------------
/**
 * @brief Stream output callback for UART-backed FILE streams.
 *
 * @param c Character to send.
 * @param stream FILE stream associated with a UART instance.
 * @return Sent character on success, or EOF on failure.
 */
extern int uartPutChar(char c, FILE *stream);

/**
 * @brief Stream input callback for UART-backed FILE streams.
 *
 * @param stream FILE stream associated with a UART instance.
 * @return Received character on success, or EOF on failure.
 */
extern int uartGetChar(FILE *stream);

/**
 * @brief Transmit a byte buffer.
 *
 * @param uart Pointer to UART instance.
 * @param buffer Source buffer containing bytes to send.
 * @param byteCount Number of bytes to transmit.
 * @return Number of bytes transmitted, or a negative error code.
 */
extern int uartTransmit(const UART_t *uart, char *buffer, size_t byteCount);

/**
 * @brief Transmit a null-terminated string.
 *
 * @param uart Pointer to UART instance.
 * @param str Null-terminated string to send.
 * @return Number of bytes transmitted, or a negative error code.
 */
extern int uartTransmitStr(const UART_t *uart, char *str);

/**
 * @brief Transmit a single character.
 *
 * @param uart Pointer to UART instance.
 * @param ch Character to transmit.
 * @return Number of bytes transmitted, or a negative error code.
 */
extern int uartTransmitChar(const UART_t *uart, char ch);

/**
 * @brief Receive bytes into a buffer.
 *
 * @param uart Pointer to UART instance.
 * @param buffer Destination buffer for received bytes.
 * @param byteCount Maximum number of bytes to receive.
 * @return Number of bytes received, or a negative error code.
 */
extern int uartReceive(const UART_t *uart, char *buffer, size_t byteCount);

/**
 * @brief Receive a single character.
 *
 * @param uart Pointer to UART instance.
 * @param character Pointer to destination character.
 * @return Number of bytes received, or a negative error code.
 */
extern int uartReceiveChar(const UART_t *uart, char *character);

/**
 * @brief Check whether the UART receive queue is empty.
 *
 * @param uart Pointer to UART instance.
 * @return True if RX queue is empty, otherwise false.
 */
extern bool uartRxEmpty(const UART_t *uart);

/**
 * @brief Check whether the UART receive queue is full.
 *
 * @param uart Pointer to UART instance.
 * @return True if RX queue is full, otherwise false.
 */
extern bool uartRxFull(const UART_t *uart);

/**
 * @brief Get UART receive queue capacity.
 *
 * @param uart Pointer to UART instance.
 * @return RX queue size in bytes.
 */
extern uint8_t uartRxSize(const UART_t *uart);

/**
 * @brief Get number of bytes currently in the UART receive queue.
 *
 * @param uart Pointer to UART instance.
 * @return Number of bytes currently queued for receive.
 */
extern uint8_t uartRxCount(const UART_t *uart);

/**
 * @brief Get high-water mark for UART receive queue usage.
 *
 * @param uart Pointer to UART instance.
 * @return Maximum observed RX queue occupancy.
 */
extern uint8_t uartRxMax(const UART_t *uart);

/**
 * @brief Check whether the UART transmit queue is empty.
 *
 * @param uart Pointer to UART instance.
 * @return True if TX queue is empty, otherwise false.
 */
extern bool uartTxEmpty(const UART_t *uart);

/**
 * @brief Check whether the UART transmit queue is full.
 *
 * @param uart Pointer to UART instance.
 * @return True if TX queue is full, otherwise false.
 */
extern bool uartTxFull(const UART_t *uart);

/**
 * @brief Get UART transmit queue capacity.
 *
 * @param uart Pointer to UART instance.
 * @return TX queue size in bytes.
 */
extern uint8_t uartTxSize(const UART_t *uart);

/**
 * @brief Get number of bytes currently in the UART transmit queue.
 *
 * @param uart Pointer to UART instance.
 * @return Number of bytes currently queued for transmit.
 */
extern uint8_t uartTxCount(const UART_t *uart);

/**
 * @brief Get high-water mark for UART transmit queue usage.
 *
 * @param uart Pointer to UART instance.
 * @return Maximum observed TX queue occupancy.
 */
extern uint8_t uartTxMax(const UART_t *uart);

/**
 * @brief Get the configured UART instance name.
 *
 * @param uart Pointer to UART instance.
 * @return Pointer to UART instance name string.
 */
extern char* uartName(const UART_t *uart);

/** @} */ // end of uart_driver

#endif /* UART_H_ */
