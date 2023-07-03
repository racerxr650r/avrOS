/*
 * uart.h
 *
 * Created: 2/19/2021 2:43:36 PM
 *  Author: admin
 */ 


#ifndef UART_H_
#define UART_H_
#include "avrOS.h"

// Data Types -----------------------------------------------------------------
typedef struct
{
	uint32_t	txBytes,rxBytes,txQueueOverflow,rxBufferOverflow,rxQueueOverflow,frameError,parityError;
}UartStats_t;

typedef struct  
{
	char			*name;
	const FILE*		file;
	USART_t			*usartRegs;
	uint32_t		baud;
	USART_PMODE_t	parity;
	USART_CHSIZE_t	dataBits;
	USART_SBMODE_t	stopBits;	
	const Queue_t	*txQueue,*rxQueue;
#ifdef UART_STATS
	UartStats_t		*stats;
#endif
}UART_t;

#define UART_FILE(uart)   uart.file

// Uart Macros -----------------------------------------------------------------
// Adds a new state machine to the list of state machines handled by the FSM manager
#ifdef UART_STATS
#define ADD_UART_RW(usartName, usartReg, uartBaud, uartParity, uartDataBits, uartStopBits, txQueueSize, rxQueueSize) \
                ADD_QUEUE(usartName ## _TxQue,txQueueSize); \
                ADD_QUEUE(usartName ## _RxQue,rxQueueSize); \
                static UartStats_t CONCAT(usartName,_stats); \
                const static UART_t SECTION(UART_TABLE) usartName; \
                static FILE CONCAT(usartName,_file) = { .put = uartPutChar, .get = uartGetChar, .flags = _FDEV_SETUP_RW, .udata = (void *)&usartName }; \
				const static UART_t SECTION(UART_TABLE) usartName = { .name = #usartName, .file = &CONCAT(usartName,_file), .usartRegs = &usartReg, .baud = uartBaud, .parity = uartParity, .dataBits = uartDataBits, .stopBits = uartStopBits, .txQueue = &CONCAT(usartName,_TxQue), .rxQueue = &CONCAT(usartName,_RxQue), .stats = &CONCAT(usartName,_stats)}; \
				ADD_STATE_MACHINE(usartName ## _SM,uartInit,FSM_DRV | 0,(void *)&usartName);
#define ADD_UART_WRITE(usartName, usartReg, uartBaud, uartParity, uartDataBits, uartStopBits, txQueueSize) \
                ADD_QUEUE(usartName ## _TxQue,txQueueSize); \
                static UartStats_t CONCAT(usartName,_stats); \
                const static UART_t SECTION(UART_TABLE) usartName; \
                static FILE CONCAT(usartName,_file) = { .put = uartPutChar, .get = uartGetChar, .flags = _FDEV_SETUP_WRITE, .udata = (void *)&usartName }; \
				const static UART_t SECTION(UART_TABLE) usartName = { .name = #usartName, .file = &CONCAT(usartName,_file), .usartRegs = &usartReg, .baud = uartBaud, .parity = uartParity, .dataBits = uartDataBits, .stopBits = uartStopBits, .txQueue = &CONCAT(usartName,_TxQue), .rxQueue = NULL, .stats = &CONCAT(usartName,_stats)}; \
				ADD_STATE_MACHINE(usartName ## _SM,uartInit,FSM_DRV | 0,(void *)&usartName);
#define ADD_UART_READ(usartName, usartReg, uartBaud, uartParity, uartDataBits, uartStopBits, rxQueueSize) \
                ADD_QUEUE(usartName ## _RxQue,rxQueueSize); \
                static UartStats_t CONCAT(usartName,_stats); \
                const static UART_t SECTION(UART_TABLE) usartName; \
                static FILE CONCAT(usartName,_file) = { .put = uartPutChar, .get = uartGetChar, .flags = _FDEV_SETUP_READ, .udata = (void *)&usartName }; \
				const static UART_t SECTION(UART_TABLE) usartName = { .name = #usartName, .file = &CONCAT(usartName,_file), .usartRegs = &usartReg, .baud = uartBaud, .parity = uartParity, .dataBits = uartDataBits, .stopBits = uartStopBits, .txQueue = NULL, .rxQueue = &CONCAT(usartName,_RxQue), .stats = &CONCAT(usartName,_stats)}; \
				ADD_STATE_MACHINE(usartName ## _SM,uartInit,FSM_DRV | 0,(void *)&usartName);
#define ADD_UART_RAW(usartName, usartReg, uartBaud, uartParity, uartDataBits, uartStopBits) \
                static UartStats_t CONCAT(usartName,_stats); \
				const static UART_t SECTION(UART_TABLE) usartName = { .name = #usartName, .file = &CONCAT(usartName,_file), .usartRegs = &usartReg, .baud = uartBaud, .parity = uartParity, .dataBits = uartDataBits, .stopBits = uartStopBits, .txQueue = NULL, .rxQueue = NULL, .stats = &CONCAT(usartName,_stats)}; \
				ADD_STATE_MACHINE(usartName ## _SM,uartInit,FSM_DRV | 0,(void *)&usartName);
#else
#define ADD_UART(usartReg, txQueue, rxQueue)	const static UART_t SECTION(UART_TABLE) uartName = { .usart = &usartReg, .txQueue = &txQueue, .rxQueue = &rxQueue};
#endif

// External Functions ---------------------------------------------------------
//extern int8_t uartInit(const UART_t *uart, uint32_t baud, USART_PMODE_t parity, USART_CHSIZE_t dataBits, USART_SBMODE_t stopBits);
//extern int uartInit(fsmStateMachine_t *stateMachine);
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
