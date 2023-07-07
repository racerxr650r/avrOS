/*
 * main.c
 *
 * Application OS resources, state machine(s), CLI commands, and main entry 
 * point. The main entry point calls sysInit(), enables interrupts, and then 
 * loops forever calling the fsmDispatch() function that implements the state
 * machine scheduler.
 *
 * Created: 2/8/2021 12:39:38 PM
 * Author : john anderson
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


// Command Line Interface Configuration ----------------------------------------
#define CLI_RX_QUEUE_SIZE	8
#define CLI_TX_QUEUE_SIZE	255
#define CLI_USART     USART2
#define CLI_BAUDRATE  115200
#define CLI_PARITY    USART_PMODE_DISABLED_gc // USART_PMODE_DISABLED_gc = No Parity
											  // USART_PMODE_EVEN_gc = Even Parity
											  // USART_PMODE_ODD_gc = Odd Parity
#define CLI_DATA_BITS USART_CHSIZE_8BIT_gc    // USART_CHSIZE_5BIT_gc = Character size: 5 bit
											  // USART_CHSIZE_6BIT_gc = Character size: 6 bit
											  // USART_CHSIZE_7BIT_gc = Character size: 7 bit
										 	  // USART_CHSIZE_8BIT_gc = Character size: 8 bit
											  // 9-bit character size is not support by the driver
										 	  // USART_CHSIZE_9BITL_gc = Character size: 9 bit read low byte first
											  // USART_CHSIZE_9BITH_gc = Character size: 9 bit read high byte first
#define CLI_STOP_BITS USART_SBMODE_1BIT_gc	  // USART_SBMODE_1BIT_gc = 1 stop bit
											  // USART_SBMODE_2BIT_gc = 2 stop bits

ADD_UART_RW(cliUart2, CLI_USART, CLI_BAUDRATE, CLI_PARITY, CLI_DATA_BITS, CLI_STOP_BITS, CLI_TX_QUEUE_SIZE, CLI_RX_QUEUE_SIZE);
ADD_CLI(cliUart2Instance,&cliUart2_file,&cliUart2_file);

// Logger Configuration --------------------------------------------------------
#define LOG_QUEUE_SIZE		255
#define LOG_USART			USART1
#define LOG_BAUDRATE		115200
#define LOG_PARITY			USART_PMODE_DISABLED_gc	// USART_PMODE_DISABLED_gc = No Parity
													// USART_PMODE_EVEN_gc = Even Parity
													// USART_PMODE_ODD_gc = Odd Parity
#define LOG_DATA_BITS		USART_CHSIZE_8BIT_gc	// USART_CHSIZE_5BIT_gc = Character size: 5 bit
													// USART_CHSIZE_6BIT_gc = Character size: 6 bit
													// USART_CHSIZE_7BIT_gc = Character size: 7 bit
													// USART_CHSIZE_8BIT_gc = Character size: 8 bit
													// 9-bit character size is not support by the driver
													// USART_CHSIZE_9BITL_gc = Character size: 9 bit read low byte first
													// USART_CHSIZE_9BITH_gc = Character size: 9 bit read high byte first
#define LOG_STOP_BITS		USART_SBMODE_1BIT_gc	// USART_SBMODE_1BIT_gc = 1 stop bit
													// USART_SBMODE_2BIT_gc = 2 stop bits

ADD_UART_WRITE(logUart1,LOG_USART,LOG_BAUDRATE, LOG_PARITY, LOG_DATA_BITS, LOG_STOP_BITS, LOG_QUEUE_SIZE);
ADD_LOG(logUart1Instance,&logUart1_file);

int main(void)
{
	// Initialize the system ---------------------------------------------------
	sysInit();
	
	INFO("Starting state machine dispatch loop");
	// Loop forever ------------------------------------------------------------
    while (1) 
    {
	    // Call the main state machine dispatcher
		fsmDispatch();
		
		// Upon return, go to sleep until the next interrupt
		sysSleep();
    }
}

