/*
 * avrOS.h
 *
 * Includes, macros, and definitions required by OS and application source
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

#ifndef AVROS_H_
#define AVROS_H_

// Toolchain/Library header files **********************************************
// avrOS.h is an umbrella header: application/module sources include it instead
// of the individual headers below. The IWYU export pragmas tell clangd's
// include-cleaner to attribute the re-exported symbols to avrOS.h, so it does
// not flag this include as "not used directly" (or suggest adding each header).
// IWYU pragma: begin_exports
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/atomic.h>
#include <avr/sleep.h>
// IWYU pragma: end_exports

// Constants *******************************************************************
#define AVRSMOS_OS

// General definitions *********************************************************
#define DISABLE 0
#define ENABLE	1

// Error codes *****************************************************************
/**
 * @brief Global status codes returned by avrOS functions.
 *
 * Functions that report only success/failure return osStatus_t directly.
 * Functions that return a count or value (e.g., bytes transferred) return a
 * non-negative count on success and a negative osStatus_t on error.
 * In both cases, the sign test `if (ret < 0)` works uniformly.
 *
 * Negative values are intentional to avoid collision with counts and to align
 * with historical avrOS convention (-1 for error). Existing code that tests
 * `if (ret < 0)` or `if (ret != 0)` will continue to work unchanged.
 */
typedef enum
{
	OS_OK          =   0,  ///< Success
	OS_ERROR       =  -1,  ///< Generic / unspecified failure
	OS_INVALID     =  -2,  ///< Invalid argument: NULL pointer, out-of-range
	OS_NOTFOUND    =  -3,  ///< Named object / handle not found
	OS_STATE       =  -4,  ///< Operation not valid in the current state
	OS_BUSY        =  -5,  ///< Resource busy / would block
	OS_EMPTY       =  -6,  ///< No data available (queue / buffer empty)
	OS_FULL        =  -7,  ///< No space available (queue / buffer full)
	OS_TIMEOUT     =  -8,  ///< Operation timed out
	OS_NORESOURCE  =  -9,  ///< Out of memory / handles / descriptors
	OS_IO          = -10,  ///< Hardware / peripheral / I/O error
	OS_UNSUPPORTED = -11,  ///< Not implemented / unsupported operation
} osStatus_t;

/// True if an osStatus_t indicates error.
#define OS_FAILED(s)    ((s) < 0)
/// True for OS_OK or a non-negative count.
#define OS_SUCCEEDED(s) ((s) >= 0)

// avrOS System Header Files ***************************************************
// IWYU pragma: begin_exports
#include "avrOSConfig.h"
#include "sys/sys.h"
#include "sys/fsm.h"
#include "sys/event.h"
#include "sys/queue.h"
#include "sys/tmr.h"
#include "srv/log.h"
#include "sys/fio.h"

#include "drv/clk.h"
#include "drv/slp.h"
#include "drv/rst.h"
#include "drv/nvm.h"
#include "drv/wdt.h"
#include "drv/cpu.h"
#include "drv/int.h"
#include "drv/mem.h"
#include "drv/uart.h"
#include "drv/spi.h"
#include "drv/twi.h"
#include "drv/vref.h"
#include "drv/dac.h"
#include "drv/adc.h"
#include "drv/ac.h"
#include "drv/zcd.h"
#include "drv/pmux.h"
#include "drv/pio.h"
#include "drv/gpio.h"
#include "drv/tca.h"
#include "drv/tcb.h"
#include "drv/rtc.h"
#include "drv/evt.h"

#include "srv/cli.h"
#include "srv/uts.h"
// IWYU pragma: end_exports
//#include "crtDrv.h"
//#include "delaySrv.h"
//#include "spiDrv.h"
//#include "sndDrv.h"
//#include "ps2Drv.h"
//#include "winSrv.h"
//#include "uiMgr.h"
//#include "tckObj.h"
//#include "txtObj.h"

// Macros *********************************************************************
#define UNUSED(x) (void)(x)

#define CONCAT_(x,y) x ## y
#define CONCAT(x,y) CONCAT_(x,y)

#define DEFAULT_OR_ARG(z,a,val,...)		val

#define CONCAT_THREE(a,b,c)				a ## b ## c
#define UNIQUENAME(prefix, func, num)	CONCAT_THREE( prefix , func, num )
#define UNIQUEIDENT(prefix)				UNIQUENAME( prefix , __FUNCTION__ , __LINE__ )

#define SECTION(sectionName)			__attribute__((__used__,__section__(#sectionName)))

#define ROM_STR(var_name,str)			static char const var_name[] PROGMEM = {str}
#define ROM_STR_G(var_name,str)			char const var_name[] PROGMEM = {str}

// Because static is confusing ************************************************
#define local      static
#define persistant static

// Inline functions *********************************************************
// Return the whole portion of the percentage representing the ratio provided
static inline int percentWhole(uint32_t den, uint32_t div)
{
	return((int)(den*100/div));
}

// Return the digits right of the decimal point of the percentage representing
// the ratio provided (Who needs bloated floating point libs?)
static inline int percentPlaces(uint32_t den, uint32_t div)
{
	return((int)((den*100%div)*100/div));
}

#endif /* AVROS_H_ */
