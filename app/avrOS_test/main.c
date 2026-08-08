/*
 * main.c
 *
 * avrOS unit test project. This project builds an image that runs the avrOS
 * unit tests instead of an application. The entry point calls sysInit() and
 * then utsRun(), which walks the TEST_TABLE, runs every test registered with
 * ADD_TEST, reports the results over the UTS report UART, and halts. It never
 * enters the FSM dispatch loop.
 *
 * Add a test by writing an osStatus_t function that returns OS_OK on success
 * or a negative osStatus_t code on failure, and registering it with ADD_TEST.
 * See doc/SDD.md sec. 5.4.
 *
 * Created: 8/2/2026
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

// AVR Fuse configuration -----------------------------------------------------
FUSES =
{
	.WDTCFG = FUSE_WDTCFG_DEFAULT,		///< Default
	.BODCFG = FUSE_BODCFG_DEFAULT,		///< Default
	.OSCCFG = FUSE_OSCCFG_DEFAULT,		///< Default
	.SYSCFG0 = 0xCC,					///< External Reset enabled on PF6
	.SYSCFG1 = FUSE_SYSCFG1_DEFAULT,	///< Default
	.CODESIZE = FUSE_CODESIZE_DEFAULT,	///< Default
	.BOOTSIZE = FUSE_BOOTSIZE_DEFAULT	///< Default
};

// AVR Lock bits configuration ------------------------------------------------
LOCKBITS = (uint8_t)LOCKBITS_DEFAULT;

// Internal function prototypes -----------------------------------------------
osStatus_t testStatusMacros(void);
osStatus_t testPercentHelpers(void);

// Unit Tests -----------------------------------------------------------------
// Each test returns OS_OK on success or a negative osStatus_t on failure. The
// tests run in the order the linker places their descriptors in TEST_TABLE.
ADD_TEST("osStatus macros", testStatusMacros);
osStatus_t testStatusMacros(void)
{
	if(!OS_FAILED(OS_ERROR))	return(OS_ERROR);
	if(!OS_SUCCEEDED(OS_OK))	return(OS_ERROR);
	if(OS_FAILED(OS_OK))		return(OS_ERROR);
	return(OS_OK);
}

ADD_TEST("percent helpers", testPercentHelpers);
osStatus_t testPercentHelpers(void)
{
	if(percentWhole(50, 100) != 50)	return(OS_ERROR);
	if(percentWhole(1, 4) != 25)	return(OS_ERROR);
	return(OS_OK);
}

// Test entry point -----------------------------------------------------------
int main(void)
{
	// Initialize the system --------------------------------------------------
	sysInit();

	// Run the registered tests, report the results, and halt (never returns) --
	utsRun();
}
