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

#define LOG_FORMAT	1	// Select the format of the log messages
						// 0 = no messages
						// 1 = Level: Message
						// 2 = System Tick: Level: Message
						// 3 = System Tick: Level: Curr State Machine: Curr State: Message
						// 4 = System Tick: Level: Src Function: Src Line: Message

//#define LOG_SERIAL		// Define to route log messages to (USART) serial port

// Log Constants
#define LOG_BANNER		CLEAR_SCREEN CURSOR_HOME RESET FG_CYAN BOLD "\r*** avrOS Logger Starting ***\n\r" RESET

// CLI Config -----------------------------------------------------------------
#define CLI_SERVICE	// Define to include the CLI state machine

// CLI constants
#define MAX_CMD_LINE    128
#define MAX_ARGS        16
#define REPEAT_SWITCH	'r'
#define CLI_BANNER		CLEAR_SCREEN CURSOR_HOME RESET FG_GREEN BOLD "\r+++| avrOS Command Line Interface |+++" RESET

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
#define EVNT_CLI    // Event commands
#define GPIO_CLI    // GPIO commands

// Service state machines to include ------------------------------------------
#undef  PCM_SERVICE

// Driver/Service Statistics
#define UART_STATS		// Calculate and track uart statistics, requires additional RAM and CPU cycles
#define QUE_STATS		// Calculate and track queue statistics, requires additional RAM and CPU cycles
#define MEM_STATS		// Fill the stack region with a pattern to track max stack size
#define EVNT_STATS      // Calculate and track event statistics
#define GPIO_STATS		// Calculate and track GPIO statistics

// System Tick timer
#define SYS_TICK_TIMER  TCB0

/*#undef WIN_STDOUT  // Stream STDOUT to root window
#undef WIN_STDERR  // Stream STDERR to status popup window (toggled by pressing F12 function key
#define SRL_STDERR	// Enable file stream/stdio to serial port
#define	SRL_STDOUT  // Stream STDOUT to serial port
#undef STDERR
#undef STDOUT*/

#endif /* AVROSCONFIG_H_ */
