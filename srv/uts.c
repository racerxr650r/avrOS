/**
 * @file uts.c
 * @brief Unit Test Service implementation — self-contained on-target runner.
 *
 * Implements the unit test service described in doc/SDD.md sec. 5.4. utsRun()
 * is called directly from the main() of a unit test project (see
 * app/avrOS_test), runs every test registered with ADD_TEST in table order,
 * reports each result over the report UART, summarizes the group, publishes the
 * outcome to utsGroupResult, and then halts. It does not use the FSM scheduler
 * and never returns.
 *
 * Output is emitted with blocking, polled UART writes, bypassing the
 * queue/fio/FSM-drained path the CLI normally uses — there is no dispatch loop
 * running to drain a transmit queue.
 *
 * Created: 7/5/2026
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
#include "../avrOS.h"

// Externs --------------------------------------------------------------------
extern void *__start_TEST_TABLE,*__stop_TEST_TABLE;

// Constants ------------------------------------------------------------------
#define UTS_USART_PTR	(&UTS_USART)	///< Report UART peripheral pointer.
#define UTS_NAME_WIDTH	28				///< Report column width for the test name.

// Globals --------------------------------------------------------------------
// Group result: 0 = not run, 1 = all passed, -1 = one or more failed.
volatile int8_t	utsGroupResult = 0;

// Per-test return codes, recorded in table order.
static osStatus_t	utsResults[UTS_MAX_TESTS];

// Internal Functions ---------------------------------------------------------
// Blocking, polled transmit of one byte over the report UART.
static void utsPutChar(char c)
{
	while(!usartDataRegisterEmpty(UTS_USART_PTR))
		;
	usartWriteData(UTS_USART_PTR, (uint8_t)c);
}

// Blocking, polled transmit of a null-terminated string.
static void utsPrint(const char *str)
{
	while(*str)
		utsPutChar(*str++);
}

// Print the test name left-justified, padded with dots to a fixed column.
static void utsPrintName(const char *name)
{
	uint8_t col = 0;

	while(name[col])
	{
		utsPutChar(name[col]);
		++col;
	}
	utsPutChar(' ');
	for(++col; col < UTS_NAME_WIDTH; ++col)
		utsPutChar('.');
	utsPutChar(' ');
}

// Minimal blocking-transmit bring-up of the report UART. Mirrors uartInit() but
// only what polled transmit needs, so the service is self-contained even in a
// build with no CLI/UART instance.
static void utsUartInit(void)
{
	USART_t		*usart = UTS_USART_PTR;
	uint16_t	freq = cpuGetFrequency();
	uint16_t	baud = UTS_BAUDRATE/100;

	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		// If the CPU frequency is calculable, derive the baud register value;
		// otherwise treat the configured value as the register value directly.
		if(freq)
			usartSetBaud(usart, ((freq/baud*10)<<2));
		else
			usartSetBaud(usart, baud);

		usartSetFrameFormat(usart, USART_CMODE_ASYNCHRONOUS_gc, UTS_PARITY, UTS_DATA_BITS, UTS_STOP_BITS);

		// Route the transmit pin (pin 0 of the USART's default port) to output.
		PORT_t *port = NULL;
		if(usart == &USART0)
			port = &PORTA;
		else if(usart == &USART1)
			port = &PORTC;
		else if(usart == &USART2)
			port = &PORTF;
		if(port != NULL)
			pioSetOutput(port, PIO_PIN0);

		usartEnableTransmitter(usart, true);
	}
}

// External Functions ---------------------------------------------------------
// Map an osStatus_t code to its short description (from the avrOS.h comments).
const char* utsStatusDescription(osStatus_t status)
{
	switch(status)
	{
		case OS_OK:          return("Success");
		case OS_ERROR:       return("Generic or unspecified failure");
		case OS_INVALID:     return("Invalid argument: NULL pointer, out-of-range value, bad enum");
		case OS_NOTFOUND:    return("Named object, handle, or device not found");
		case OS_STATE:       return("Operation not valid in the current state");
		case OS_BUSY:        return("Resource busy / would block");
		case OS_EMPTY:       return("No data available (queue / buffer empty)");
		case OS_FULL:        return("No space available (queue / buffer full)");
		case OS_TIMEOUT:     return("Operation timed out");
		case OS_NORESOURCE:  return("Out of memory / handles / descriptors");
		case OS_IO:          return("Hardware / peripheral / I/O error");
		case OS_UNSUPPORTED: return("Not implemented / unsupported operation");
		default:             return("Unknown status code");
	}
}

// Run all registered unit tests and halt.
void utsRun(void)
{
	// Bring up the report UART for blocking, polled output.
	utsUartInit();

	utsPrint("\r\n" BOLD FG_GREEN "*** avrOS Unit Tests ***" RESET "\r\n");

	// Walk the test table, running and reporting each test in order.
	test_t		*test = (test_t *)&__start_TEST_TABLE;
	uint16_t	count = 0;
	for(; test < (test_t *)&__stop_TEST_TABLE && count < UTS_MAX_TESTS; ++test, ++count)
	{
		osStatus_t result = test->func ? test->func() : OS_ERROR;
		utsResults[count] = result;

		utsPrintName(test->name);
		if(result == OS_OK)
			utsPrint("[" FG_GREEN "PASS" RESET "] ");
		else
			utsPrint("[" FG_RED "FAIL" RESET "] ");
		utsPrint(utsStatusDescription(result));
		utsPrint("\r\n");
	}

	// Walk the recorded return codes to decide the group result.
	bool allPassed = true;
	for(uint16_t i = 0; i < count; ++i)
		if(utsResults[i] != OS_OK)
			allPassed = false;

	if(allPassed)
	{
		utsPrint("Test Group [" FG_GREEN "PASSED" RESET "]\r\n");
		utsGroupResult = 1;
	}
	else
	{
		utsPrint("Test Group [" FG_RED "FAIL" RESET "]\r\n");
		utsGroupResult = -1;
	}

	// Halt so the report and utsGroupResult remain observable.
	while(1)
		;
}
