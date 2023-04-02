/*
 * avrOSConfig.h
 *
 * Configuration of the avrOS resources
 *
 * Created: 4/25/2021 1:36:42 PM
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

#ifndef AVROSCONFIG_H_
#define AVROSCONFIG_H_

// CPU Config -----------------------------------------------------------------
// Set the CPU speed using the internal high speed oscillator
#define CPU_SPEED	CLKCTRL_FRQSEL_24M_gc	// CLKCTRL_FRQSEL_1M_gc = 1 MHz system clock
											// CLKCTRL_FRQSEL_2M_gc = 2 MHz system clock
											// CLKCTRL_FRQSEL_3M_gc = 3 MHz system clock
											// CLKCTRL_FRQSEL_4M_gc = 4 MHz system clock (default)
											// CLKCTRL_FRQSEL_8M_gc = 8 MHz system clock
											// CLKCTRL_FRQSEL_12M_gc = 12 MHz system clock
											// CLKCTRL_FRQSEL_16M_gc = 16 MHz system clock
											// CLKCTRL_FRQSEL_20M_gc = 20 MHz system clock
											// CLKCTRL_FRQSEL_24M_gc = 24 MHz system clock

// Logger Output (STDERR) -----------------------------------------------------
#define LOG_LEVEL	4	// Enable log messages by level of severity
						// 0 = no messages
						// 1 = critical messages
						// 2 = error and critical messages
						// 3 = warning, error, and critical messages
						// 4 = info, warning, error, and critical messages

#define LOG_FORMAT	2	// Select the format of the log messages
						// 0 = no messages
						// 1 = System Tick: Level: Message
						// 2 = System Tick: Level: Curr State Machine: Curr State: Message
						// 3 = System Tick: Level: Src Function: Src Line: Message

#define LOG_SERIAL		// Define to route log messages to (USART) serial port

// Log Constants
#define LOG_BANNER			"\n\r*** avrOS Logger Starting ***\n\r"

// LOG interface config
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
										
// CLI Config -----------------------------------------------------------------
#define CLI_SERVICE	// Define to include the CLI state machine

// CLI constants
#define MAX_CMD_LINE    128
#define MAX_ARGS        16
#define REPEAT_SWITCH	'r'
#define CLI_BANNER			"\n\ravrOS Command Line Interface\n\r"

// Driver/Service CLI command(s)
#define UART_CLI	// Uart driver CLI commands
#define QUE_CLI		// Queue service commands
#define FSM_CLI		// Finite State Machine service commands
#define CLI_CLI		// Command line interface service commands
#define TICK_CLI	// System tick commands
#define CPU_CLI		// CPU commands
#define TMR_CLI		// Timer commands
#define MEM_CLI		// Memory commands
#define CPU_CLI		// CPU commands

// CLI interface config
#define CLI_RX_QUEUE_SIZE	8
#define CLI_TX_QUEUE_SIZE	255
#define CLI_BAUDRATE		115200
#define CLI_USART			USART2
#define CLI_PARITY			USART_PMODE_DISABLED_gc	// USART_PMODE_DISABLED_gc = No Parity
													// USART_PMODE_EVEN_gc = Even Parity
													// USART_PMODE_ODD_gc = Odd Parity
#define CLI_DATA_BITS		USART_CHSIZE_8BIT_gc	// USART_CHSIZE_5BIT_gc = Character size: 5 bit
													// USART_CHSIZE_6BIT_gc = Character size: 6 bit
													// USART_CHSIZE_7BIT_gc = Character size: 7 bit
													// USART_CHSIZE_8BIT_gc = Character size: 8 bit
													// 9-bit character size is not support by the driver
													// USART_CHSIZE_9BITL_gc = Character size: 9 bit read low byte first
													// USART_CHSIZE_9BITH_gc = Character size: 9 bit read high byte first
#define CLI_STOP_BITS		USART_SBMODE_1BIT_gc	// USART_SBMODE_1BIT_gc = 1 stop bit
													// USART_SBMODE_2BIT_gc = 2 stop bits

// Service state machines to include ------------------------------------------
#define PCM_SERVICE

// Driver/Service Statistics
#define UART_STATS		// Calculate and track uart statistics, requires additional RAM and CPU cycles
#define QUE_STATS		// Calculate and track queue statistics, requires additional RAM and CPU cycles
#define MEM_STATS		// Fill the stack region with a pattern to track max stack size

// System Tick timer
#define SYS_TICK_TIMER  TCB0

/*#undef WIN_STDOUT  // Stream STDOUT to root window
#undef WIN_STDERR  // Stream STDERR to status popup window (toggled by pressing F12 function key
#define SRL_STDERR	// Enable file stream/stdio to serial port
#define	SRL_STDOUT  // Stream STDOUT to serial port
#undef STDERR
#undef STDOUT*/

#endif /* AVROSCONFIG_H_ */