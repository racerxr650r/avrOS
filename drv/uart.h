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
	const char		*name;
	USART_t			*usart;
	const Queue_t	*txQueue,*rxQueue;
#ifdef UART_STATS
	UartStats_t		*stats;
#endif
}UART_t;

// Adds a new state machine to the list of state machines handled by the FSM manager
#ifdef UART_STATS
#define ADD_UART(uartName, usartReg, txQueueName, rxQueueName)	static UartStats_t CONCAT(uartName,_stats); \
																const static UART_t SECTION(UART_TABLE) uartName = {.name = #uartName, .usart = &usartReg, .txQueue = txQueueName, .rxQueue = rxQueueName, .stats = &CONCAT(uartName,_stats)};
#else
#define ADD_UART(uartName, usartReg, txQueue, rxQueue)	const static UART_t SECTION(UART_TABLE) uartName = {.name = #uartName, .usart = &usartReg, .txQueue = &txQueue, .rxQueue = &rxQueue};
#endif

// External Functions ---------------------------------------------------------
extern int8_t uartInit(const UART_t *uart, uint32_t baud, USART_PMODE_t parity, USART_CHSIZE_t dataBits, USART_SBMODE_t stopBits);
extern int uartPutChar(char c, FILE *stream);
extern int uartTransmit(const UART_t *uart, char *buffer, size_t byteCount);
extern int uartTransmitStr(const UART_t *uart, char *str);
extern int uartTransmitChar(const UART_t *uart, char ch);
extern int uartReceive(const UART_t *uart, char *buffer, size_t byteCount);
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

#endif /* UART_H_ */
