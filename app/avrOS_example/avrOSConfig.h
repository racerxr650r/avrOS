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

#define LOG_FORMAT	3	// Select the format of the log messages
						// 0 = no messages
						// 1 = Level: Message
						// 2 = System Tick: Level: Message
						// 3 = System Tick: Level: Curr State Machine: Curr State: Message
						// 4 = System Tick: Level: Src Function: Src Line: Message
// Log Constants
#define LOG_BANNER		CLEAR_SCREEN CURSOR_HOME RESET FG_CYAN BOLD "\r*** avrOS Logger Starting ***\n\r" RESET
#define DISPLAY_PROMPT	FG_GREEN "\ravrOS> " RESET

// CLI Config -----------------------------------------------------------------
// CLI constants
#define MAX_CMD_LINE    128
#define MAX_ARGS        16
#define REPEAT_SWITCH	'r'
#define CLI_BANNER		CLEAR_SCREEN CURSOR_HOME RESET FG_GREEN BOLD "\r+++| avrOS Command Line Interface |+++" RESET

// Global CLI enable
#define CLI
// Driver/Service CLI command(s)
#ifdef CLI
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
#else
#undef UART_CLI		// Uart driver CLI commands
#undef QUE_CLI		// Queue service commands
#undef FSM_CLI		// Finite State Machine service commands
#undef CLI_CLI		// Command line interface service commands
#undef TICK_CLI		// System tick commands
#undef CPU_CLI		// CPU commands
#undef TMR_CLI		// Timer commands
#undef MEM_CLI		// Memory commands
#undef CPU_CLI		// CPU commands
#undef EVNT_CLI		// Event commands
#undef GPIO_CLI		// GPIO commands
#endif

// Service state machines to include ------------------------------------------
#undef  PCM_SERVICE

// Driver/Service Statistics
#define FSM_STATS	    // Include string names of state machines and states
#define UART_STATS		// Calculate and track uart statistics, requires additional RAM and CPU cycles
#define QUE_STATS		// Calculate and track queue statistics, requires additional RAM and CPU cycles
#define MEM_STATS		// Fill the stack region with a pattern to track max stack size
#define EVNT_STATS      // Calculate and track event statistics
#define GPIO_STATS		// Calculate and track GPIO statistics

// System Tick timer
#define SYS_TICK_TIMER  TCB0

#endif /* AVROSCONFIG_H_ */
