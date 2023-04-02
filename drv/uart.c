/*
 * uart.c
 *
 * Implement a standard asynchronous receiver/transmitter using the AVR-Dx USART
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
#include "avrOS.h"

// Externs --------------------------------------------------------------------
extern void *__start_UART_TABLE,*__stop_UART_TABLE;

// Internal Variable ----------------------------------------------------------
static UART_t *uart0 = NULL, *uart1 = NULL, *uart2 = NULL;

// Internal Function Prototypes -----------------------------------------------
static void isrUsartDRE(UART_t *uart);
static void isrUsartRXC(UART_t *uart);

// Interrupt Table Hooks ------------------------------------------------------
// Hook the USART0 Data Register Empty interrupt handler
ISR(USART0_DRE_vect)
{
	if(uart0 != NULL)
		isrUsartDRE(uart0);
}
// Hook the USART1 Data Register Empty interrupt handler
ISR(USART1_DRE_vect)
{
	if(uart1 != NULL)
		isrUsartDRE(uart1);
}
// Hook the USART2 Data Register Empty interrupt handler
ISR(USART2_DRE_vect)
{
	if(uart2 != NULL)
		isrUsartDRE(uart2);
}
// Hook the USART0 Receive Complete interrupt handler
ISR(USART0_RXC_vect)
{
	if(uart0 != NULL)
		isrUsartRXC(uart0);
}
// Hook the USART1 Receive Complete interrupt handler
ISR(USART1_RXC_vect)
{
	if(uart1 != NULL)
		isrUsartRXC(uart1);
}
// Hook the USART2 Receive Complete interrupt handler
ISR(USART2_RXC_vect)
{
	if(uart2 != NULL)
		isrUsartRXC(uart2);
}

// Interrupt Handler Functions ------------------------------------------------
// USART Data Register Empty (Tx) interrupt handler
static void isrUsartDRE(UART_t *uart)
{
	uint8_t		txByte;
	
	// If there are more bytes in the transmit queue...
	if(queGet(uart->txQueue,(char *)&txByte))
	{
		uart->usart->TXDATAL = txByte;
#ifdef UART_STATS
		++uart->stats->txBytes;
#endif
	}
	// Else transmit queue is empty...
	else
	{
		// Disable the tx data register empty interrupt
		uart->usart->CTRLA &= ~USART_DREIE_bm;
	}
}

// USART Receive Complete (Rx) interrupt handler
static void isrUsartRXC(UART_t *uart)
{
	uint8_t		error = uart->usart->RXDATAH;
	
	if(error == USART_RXCIF_bm)
	{
		bool queued = quePut(uart->rxQueue,uart->usart->RXDATAL);
#ifdef UART_STATS
		if(queued)
			++uart->stats->rxBytes;
		else
			++uart->stats->rxQueueOverflow;
#endif
	}
#ifdef UART_STATS
	else if(error&USART_PERR_bm)
		++uart->stats->parityError;
	else if(error&USART_FERR_bm)
		++uart->stats->frameError;
	else if(error&USART_BUFOVF_bm)
		++uart->stats->rxBufferOverflow;
#endif
}

// Command Line Interface -----------------------------------------------------
#ifdef UART_CLI
ADD_COMMAND("uart",uartCmd,true);
#endif

int uartCmd(int argc, char *argv[])
{
	// Walk the table of uarts
	UART_t *uart = (UART_t *)&__start_UART_TABLE;
	for(; uart < (UART_t *)&__stop_UART_TABLE; ++uart)
	{
		if(argc<2 || (argc==2 && !strcmp(uart->name,argv[1])))
		{
			printf("UART: %s\n\r",uart->name);
			printf("\tTx:%8lu Rx:%8lu Frame Err:%8lu Parity Err:%8lu\n\r",uart->stats->txBytes,uart->stats->rxBytes,uart->stats->frameError,uart->stats->parityError);
			printf("\tTxOvrflw:%8lu RxBufferOvrflw:%8lu RxQueueOvrflw:%8lu\n\r",uart->stats->txQueueOverflow,uart->stats->rxBufferOverflow,uart->stats->rxQueueOverflow);
		}
	}
	return(0);
}

// External Functions ---------------------------------------------------------

// Initialize a given USART to operate as a UART with the given operating
// parameters.
// Return: 0 - No error, -1 - Invalid USART specified
int8_t uartInit(const UART_t	*uart,
				uint32_t		baud,
				USART_PMODE_t	parity,
				USART_CHSIZE_t	dataBits,
				USART_SBMODE_t	stopBits)
{
	USART_t		*usart = uart->usart;
	int8_t		ret = 0;
	uint32_t	freq = cpuGetFrequency();

#ifdef UART_STATS
	// Zero out the interface stats
	memset((void *)&uart->stats,0,sizeof(uart->stats));
#endif	

	// Disable interrupts while setting up the USART and restore interrupts to previous state
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		// If the CPU frequency is calculable...
		if(freq)
			usart->BAUD = (uint16_t)((freq/baud)<<2);
		// Else baud is the baud register value
		else
			usart->BAUD = baud;

		// Set the mode as async and frame format as specified
		usart->CTRLC = USART_CMODE_ASYNCHRONOUS_gc | parity | stopBits | dataBits;

		// Set the pin associated with Tx to output
		PORT_t *port=NULL;
		// If USART0...
		if(usart == &USART0)
		{
			uart0 = (UART_t *)uart;
			port = &PORTA;
		}
		// If USART1...
		else if(usart == &USART1)
		{
			uart1 = (UART_t *)uart;
			port = &PORTC;
		}
		// If USART2...
		else if(usart == &USART2)
		{
			uart2 = (UART_t *)uart;
			port = &PORTF;
		}
		// If the port has been located...
		if(port != NULL)
			// Set the port direction pin (output)
			port->DIRSET = 0b00000001;
		// Port unknown...
		else
			// Return error
			ret = -1;
		
		// If no errors...	
		if(!ret)
		{
			// If Rx queues exist, enable Rx interrupts
			if(uart->rxQueue!=NULL)
				usart->CTRLA |= USART_RXCIE_bm;

			// Enable the Tx, Rx, and standard Rx mode (16 over samples)
			usart->CTRLB |= USART_RXEN_bm | USART_TXEN_bm | USART_RXMODE_NORMAL_gc;
		}
	}
	
	return(ret);
}

// Imlpement putchar() to support stdio buffered file I/O
int uartPutChar(char c, FILE *stream)
{
	int		ret;
	UART_t	*uart = (UART_t *)fdev_get_udata(stream);
	
	if(uartTransmitChar(uart,c))
	ret = 0;
	else
	{
		ERROR("UART xmit buffer full");
		ret = -1;
	}
	
	return(ret);
}

// Load the uart transmit queue with the number of bytes specified. If 
// byteCount is greater than the size of the queue remaining, the queue
// is filled and the number of bytes loaded into the queue is returned.
// Return: The number of bytes actually loaded into the queue.
int uartTransmit(const UART_t *uart, char *buffer, size_t byteCount)
{
	int i=0; 

	// If there is a transmit queue...
	if(uart->txQueue != NULL)
 	{
		for(;i<byteCount;++i)
		{
			// If the buffer is full...
			if(!quePut(uart->txQueue,buffer[i]))
			{
#ifdef UART_STATS
				// Increment the buffer overflow counter
				++uart->stats->txQueueOverflow;
#endif				
				break;
			}
		}
		// Enable the tx data register empty interrupt
		uart->usart->CTRLA |= USART_DREIE_bm;
	}
	// Else there is no transmit queue...
	else
	{
		uart->usart->TXDATAL = buffer[i++];
#ifdef UART_STATS
		// Increment the tx byte counter
		++uart->stats->txBytes;
#endif
	}
	return(i);
}

int uartTransmitStr(const UART_t *uart, char *str)
{
	return(uartTransmit(uart,str,strlen(str)));
}

int uartTransmitChar(const UART_t *uart, char ch)
{
	return(uartTransmit(uart,&ch,1));
}

// Copy up to "byteCount" bytes from the uart receive queue. If no bytes
// are currently in the queue, no bytes are copied and 0 is returned. If there 
// are fewer bytes in the queue than byteCount requests, the existing bytes are 
// copied and the number is returned. If more bytes are in the queue than 
// requested, the number of bytes requested is copied to the buffer and that
// number is returned.
// Return: The number of bytes actually copied to buffer
int uartReceive(const UART_t *uart, char *buffer, size_t byteCount)
{
	int i = 0;

	// If the UART has an RX queue...
	if(uart->rxQueue != NULL)
	{
		// Up to byteCount times...
		for(;i<byteCount;++i)
		{
			char ch;
			
			// If there is a byte in the queue...
			if(queGet(uart->rxQueue,&ch))
				// Copy the byte to the next buffer location
				buffer[i] = ch;
			// Else no more bytes in the queue...
			else
				// Stop checking the queue
				break;
		}
	}
	// Else if the UART does not have a RX queue and the Receive Complete Flag is set...
	else if(uart->usart->RXDATAH & USART_RXCIF_bm)
	{
		// Copy the 
		buffer[0] = uart->usart->RXDATAL;
		++i;
	}
	
	return(i);
}

bool uartRxEmpty(const UART_t *uart)
{
	return(queEmpty(uart->rxQueue));
}

bool uartRxFull(const UART_t *uart)
{
	return(queFull(uart->rxQueue));
}

uint8_t uartRxSize(const UART_t *uart)
{
	return(queSize(uart->rxQueue));
}

uint8_t uartRxCount(const UART_t *uart)
{
	return(queCount(uart->rxQueue));
}

uint8_t uartRxMax(const UART_t *uart)
{
	return(queMax(uart->rxQueue));
}

bool uartTxEmpty(const UART_t *uart)
{
	return(queEmpty(uart->txQueue));
}

bool uartTxFull(const UART_t *uart)
{
	return(queFull(uart->txQueue));
}

uint8_t uartTxSize(const UART_t *uart)
{
	return(queSize(uart->txQueue));
}

uint8_t uartTxCount(const UART_t *uart)
{
	return(queCount(uart->txQueue));
}

uint8_t uartTxMax(const UART_t *uart)
{
	return(queMax(uart->txQueue));
}
