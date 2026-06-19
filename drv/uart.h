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
