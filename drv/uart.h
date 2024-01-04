/*
 * uart.h
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
#include "avrOS.h"

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
#define UART_FILE_PTR(uartName)	CONCAT(uartName,_file)

#ifdef UART_STATS
// Verbose version of the UART_ADD macros. These versions of the macros save
// the name of the UART and create statistics that can be used by the
// application, cli, or modbus server
/**----------------------------------------------------------------------------
 * Adds a Send/Receive UART to the system
 * 
 * Adds a send/receive UART to the system with the name provided in the first
 * parameter. The name provided is used to create an instance of UART_t. This
 * instance is used for all UART related functions. In addition, a FILE stream
 * is created with this name appended with _file. This FILE pointer can be used
 * with C stdio functions like fprintf and fgetc to read and write to the port.
 * The second parameter specifies the CPU UART (USART0, USART1,
 * or USART2) to use. The next parameters specify the baud rate, parity, data
 * bits, and stop bits. The most common settings for device uarts is
 * 115200,N,8,1. The last two parameters specify the size of the send and
 * receive queues. This macro will also create the queues. Lastly, the macro
 * adds a UART initialization function call to the global finite state machine
 * table. This initialization will be called during execution of sysInit().
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
/**----------------------------------------------------------------------------
 * Adds a Send only UART to the system
 * 
 * Adds a send only UART to the system with the name provided in the first
 * parameter. The name provided is used to create an instance of UART_t. This
 * instance is used for all UART related functions. In addition, a FILE stream
 * is created with this name appended with _file. This FILE pointer can be used
 * with C stdio functions like fprintf and fputc to write to the port.
 * The second parameter specifies the CPU UART (USART0, USART1,
 * or USART2) to use. The next parameters specify the baud rate, parity, data
 * bits, and stop bits. The most common settings for device uarts is
 * 115200,N,8,1. The last parameter specifies the size of the send queue. This 
 * macro will also create the queue. Lastly, the macro adds a UART
 * initialization function call to the global finite state machine table. This
 * initialization will be called during execution of sysInit().
*/
#define ADD_UART_WRITE(usartName, usartReg, uartBaud, uartParity, uartDataBits, uartStopBits, txQueueSize) \
				ADD_QUEUE(usartName ## _TxQue,sizeof(uint8_t),txQueueSize); \
				static UartStats_t CONCAT(usartName,_stats); \
				const static UART_t SECTION(UART_TABLE) usartName; \
				const static fioBuffers_t CONCAT(usartName,_buffers) = {.input = NULL, .output = &CONCAT(usartName,_TxQue)}; \
				static FILE CONCAT(usartName,_file) = {.buf = (char *) &CONCAT(usartName,_buffers), .put = uartPutChar, .get = uartGetChar, .flags = _FDEV_SETUP_WRITE, .udata = (void *)&usartName }; \
				const static UART_t SECTION(UART_TABLE) usartName = { .name = #usartName, .file = &CONCAT(usartName,_file), .usartRegs = &usartReg, .baud = (uartBaud/100), .parity = uartParity, .dataBits = uartDataBits, .stopBits = uartStopBits, .txQueue = &CONCAT(usartName,_TxQue), .rxQueue = NULL, .stats = &CONCAT(usartName,_stats)}; \
				ADD_INITIALIZER(usartName,uartInit,(void *)&usartName);
/**----------------------------------------------------------------------------
 * Adds a receive only UART to the system
 * 
 * Adds a receive only UART to the system with the name provided in the first
 * parameter. The name provided is used to create an instance of UART_t. This
 * instance is used for all UART related functions. In addition, a FILE stream
 * is created with this name appended with _file. This FILE pointer can be used
 * with C stdio functions like fscanf and fgetc to write to the port. The
 * second parameter specifies the CPU UART (USART0, USART1, or USART2) to use.
 * The next parameters specify the baud rate, parity, data bits, and stop bits.
 * The most common settings for device uarts is 115200,N,8,1. The last
 * parameter specifies the size of the receive queue. This macro will also 
 * create the queue. Lastly, the macro adds a UART initialization function call
 * to the global finite state machine table. This initialization will be called
 * during execution of sysInit().
*/
#define ADD_UART_READ(usartName, usartReg, uartBaud, uartParity, uartDataBits, uartStopBits, rxQueueSize) \
				ADD_QUEUE(usartName ## _RxQue,sizeof(uint8_t),rxQueueSize); \
				static UartStats_t CONCAT(usartName,_stats); \
				const static UART_t SECTION(UART_TABLE) usartName; \
				static FILE CONCAT(usartName,_file) = { .put = uartPutChar, .get = uartGetChar, .flags = _FDEV_SETUP_READ, .udata = (void *)&usartName }; \
				const static UART_t SECTION(UART_TABLE) usartName = { .name = #usartName, .file = &CONCAT(usartName,_file), .usartRegs = &usartReg, .baud = (uartBaud/100), .parity = uartParity, .dataBits = uartDataBits, .stopBits = uartStopBits, .txQueue = NULL, .rxQueue = &CONCAT(usartName,_RxQue), .stats = &CONCAT(usartName,_stats)}; \
				ADD_INITIALIZER(usartName,uartInit,(void *)&usartName);
/**----------------------------------------------------------------------------
 * Adds a raw UART to the system
 * 
 * Adds a raw UART to the system with the name provided in the first
 * parameter. The name provided is used to create an instance of UART_t. This
 * instance is used for all UART related functions. There are no queues or
 * FILE i/o stream instance created for this UART. The user application will
 * have to implement any buffing required. The second parameter specifies the
 * CPU UART (USART0, USART1, or USART2) to use. The next parameters specify the
 * baud rate, parity, data bits, and stop bits. The most common settings for
 * device uarts is 115200,N,8,1. Lastly, the macro adds a UART initialization
 * function call to the global finite state machine table. This initialization
 * will be called during execution of sysInit().
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
extern int uartPutChar(char c, FILE *stream);
extern int uartGetChar(FILE *stream);
extern int uartTransmit(const UART_t *uart, char *buffer, size_t byteCount);
extern int uartTransmitStr(const UART_t *uart, char *str);
extern int uartTransmitChar(const UART_t *uart, char ch);
extern int uartReceive(const UART_t *uart, char *buffer, size_t byteCount);
extern int uartReceiveChar(const UART_t *uart, char *character);
extern bool uartRxEmpty(const UART_t *uart);
extern bool uartRxFull(const UART_t *uart);
extern uint8_t uartRxSize(const UART_t *uart);
extern uint8_t uartRxCount(const UART_t *uart);
extern uint8_t uartRxMax(const UART_t *uart);
extern bool uartTxEmpty(const UART_t *uart);
extern bool uartTxFull(const UART_t *uart);
extern uint8_t uartTxSize(const UART_t *uart);
extern uint8_t uartTxCount(const UART_t *uart);
extern uint8_t uartTxMax(const UART_t *uart);
extern char* uartName(const UART_t *uart);

#endif /* UART_H_ */
